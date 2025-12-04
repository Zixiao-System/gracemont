/*
 * Gracemont Industrial Control Framework - ARM64 RTOS
 * Copyright (C) 2024 Zixiao System <https://github.com/Zixiao-System>
 *
 * This file is part of Gracemont Industrial Control Framework.
 * Repository: https://github.com/Zixiao-System/gracemont
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Lightweight TCP/IP Stack Implementation
 */

#include "net_stack.h"
#include "rtos_config.h"
#include "rtos_types.h"

extern void sem_post(semaphore_t *sem);

/* Network Interface List */
static netif_t *netif_list = NULL;
static netif_t *netif_default = NULL;
static spinlock_t netif_lock = SPINLOCK_INIT;

/* Socket Table */
socket_t *socket_table[CONFIG_NET_MAX_SOCKETS];
spinlock_t socket_lock = SPINLOCK_INIT;
static int next_fd __attribute__((unused)) = 0;

/* ARP Cache */
#define ARP_CACHE_SIZE  64
typedef struct {
    uint32_t    ip;
    uint8_t     mac[6];
    tick_t      expire;
    bool        valid;
} arp_entry_t;

static arp_entry_t arp_cache[ARP_CACHE_SIZE];
static spinlock_t arp_lock = SPINLOCK_INIT;

/* TCP Connections */
static socket_t *tcp_listen_list __attribute__((unused)) = NULL;
static socket_t *tcp_conn_list __attribute__((unused)) = NULL;
static spinlock_t tcp_lock __attribute__((unused)) = SPINLOCK_INIT;

/*
 * Byte Order Functions
 */
uint16_t htons(uint16_t h)
{
    return ((h & 0xFF) << 8) | ((h >> 8) & 0xFF);
}

uint32_t htonl(uint32_t h)
{
    return ((h & 0xFF) << 24) | ((h & 0xFF00) << 8) |
           ((h >> 8) & 0xFF00) | ((h >> 24) & 0xFF);
}

uint16_t ntohs(uint16_t n) { return htons(n); }
uint32_t ntohl(uint32_t n) { return htonl(n); }

/*
 * Internet Checksum
 */
uint16_t inet_checksum(const void *data, size_t len)
{
    const uint16_t *ptr = (const uint16_t *)data;
    uint32_t sum = 0;

    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }

    if (len == 1) {
        sum += *(const uint8_t *)ptr;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

uint16_t inet_pseudo_checksum(uint32_t src, uint32_t dst, uint8_t proto, uint16_t len)
{
    uint32_t sum = 0;
    sum += (src >> 16) & 0xFFFF;
    sum += src & 0xFFFF;
    sum += (dst >> 16) & 0xFFFF;
    sum += dst & 0xFFFF;
    sum += htons(proto);
    sum += htons(len);

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return sum;
}

/*
 * Network Stack Initialization
 */
void net_stack_init(void)
{
    /* Initialize ARP cache */
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_cache[i].valid = false;
    }

    /* Initialize socket table */
    for (int i = 0; i < CONFIG_NET_MAX_SOCKETS; i++) {
        socket_table[i] = NULL;
    }
}

/*
 * Network Interface Management
 */
status_t netif_register(netif_t *nif)
{
    spin_lock_irq(&netif_lock);

    nif->next = netif_list;
    netif_list = nif;

    if (netif_default == NULL) {
        netif_default = nif;
    }

    spin_unlock_irq(&netif_lock);
    return STATUS_OK;
}

netif_t *netif_get_default(void)
{
    return netif_default;
}

/*
 * Ethernet Output
 */
static status_t eth_output(netif_t *nif, zbuf_t *zb, const uint8_t *dst_mac, uint16_t type)
{
    /* Push Ethernet header */
    eth_hdr_t *eth = (eth_hdr_t *)zbuf_push(zb, ETH_HDR_LEN);
    if (eth == NULL) {
        zbuf_free(zb);
        return STATUS_NO_MEM;
    }

    /* Fill header */
    for (int i = 0; i < 6; i++) {
        eth->dst[i] = dst_mac[i];
        eth->src[i] = nif->mac[i];
    }
    eth->type = htons(type);

    /* Set protocol offsets */
    zb->l2_offset = 0;
    zb->protocol = type;

    /* Send via driver */
    nif->tx_packets++;
    nif->tx_bytes += zb->len;

    return nif->send(nif, zb);
}

/*
 * Network Input Handler
 */
void netif_input(netif_t *nif, zbuf_t *zb)
{
    if (zb->len < ETH_HDR_LEN) {
        zbuf_free(zb);
        return;
    }

    eth_hdr_t *eth = (eth_hdr_t *)zb->data;
    uint16_t type = ntohs(eth->type);

    /* Set layer offsets */
    zb->l2_offset = 0;
    zb->l3_offset = ETH_HDR_LEN;
    zb->protocol = type;
    zb->netif = nif;

    /* Pull Ethernet header */
    zbuf_pull(zb, ETH_HDR_LEN);

    nif->rx_packets++;
    nif->rx_bytes += zb->len;

    /* Dispatch by protocol */
    switch (type) {
    case ETH_TYPE_IP:
        ip_input(nif, zb);
        break;
    case ETH_TYPE_ARP:
        arp_input(nif, zb);
        break;
    case ETH_TYPE_PNIO:
        /* PROFINET - handled separately */
        zbuf_free(zb);
        break;
    default:
        zbuf_free(zb);
        break;
    }
}

/*
 * ARP Functions
 */
void arp_input(netif_t *nif, zbuf_t *zb)
{
    if (zb->len < sizeof(arp_hdr_t)) {
        zbuf_free(zb);
        return;
    }

    arp_hdr_t *arp = (arp_hdr_t *)zb->data;

    /* Only handle Ethernet/IPv4 */
    if (ntohs(arp->htype) != 1 || ntohs(arp->ptype) != ETH_TYPE_IP) {
        zbuf_free(zb);
        return;
    }

    uint32_t spa = ntohl(arp->spa);
    uint32_t tpa = ntohl(arp->tpa);

    /* Update ARP cache */
    spin_lock(&arp_lock);
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!arp_cache[i].valid) {
            arp_cache[i].ip = spa;
            for (int j = 0; j < 6; j++) {
                arp_cache[i].mac[j] = arp->sha[j];
            }
            arp_cache[i].expire = get_system_ticks() + 300000; /* 5 min */
            arp_cache[i].valid = true;
            break;
        }
    }
    spin_unlock(&arp_lock);

    /* Check if request is for us */
    if (ntohs(arp->oper) == ARP_OP_REQUEST && tpa == nif->ip) {
        /* Send reply */
        zbuf_t *reply = zbuf_alloc_tx(sizeof(arp_hdr_t));
        if (reply != NULL) {
            arp_hdr_t *rep = (arp_hdr_t *)zbuf_put(reply, sizeof(arp_hdr_t));
            rep->htype = htons(1);
            rep->ptype = htons(ETH_TYPE_IP);
            rep->hlen = 6;
            rep->plen = 4;
            rep->oper = htons(ARP_OP_REPLY);
            for (int i = 0; i < 6; i++) {
                rep->sha[i] = nif->mac[i];
                rep->tha[i] = arp->sha[i];
            }
            rep->spa = htonl(nif->ip);
            rep->tpa = arp->spa;

            eth_output(nif, reply, arp->sha, ETH_TYPE_ARP);
        }
    }

    zbuf_free(zb);
}

status_t arp_resolve(uint32_t ip, uint8_t *mac)
{
    /* Check cache */
    spin_lock(&arp_lock);
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            for (int j = 0; j < 6; j++) {
                mac[j] = arp_cache[i].mac[j];
            }
            spin_unlock(&arp_lock);
            return STATUS_OK;
        }
    }
    spin_unlock(&arp_lock);

    /* Send ARP request */
    netif_t *nif = netif_default;
    if (nif == NULL) return STATUS_ERROR;

    zbuf_t *zb = zbuf_alloc_tx(sizeof(arp_hdr_t));
    if (zb == NULL) return STATUS_NO_MEM;

    arp_hdr_t *arp = (arp_hdr_t *)zbuf_put(zb, sizeof(arp_hdr_t));
    arp->htype = htons(1);
    arp->ptype = htons(ETH_TYPE_IP);
    arp->hlen = 6;
    arp->plen = 4;
    arp->oper = htons(ARP_OP_REQUEST);
    for (int i = 0; i < 6; i++) {
        arp->sha[i] = nif->mac[i];
        arp->tha[i] = 0xFF;
    }
    arp->spa = htonl(nif->ip);
    arp->tpa = htonl(ip);

    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    eth_output(nif, zb, broadcast, ETH_TYPE_ARP);

    return STATUS_WOULD_BLOCK;
}

/*
 * IP Functions
 */
void ip_input(netif_t *nif, zbuf_t *zb)
{
    if (zb->len < sizeof(ip_hdr_t)) {
        zbuf_free(zb);
        return;
    }

    ip_hdr_t *ip = (ip_hdr_t *)zb->data;
    uint8_t ihl = IP_HDR_LEN(ip);

    /* Verify checksum */
    if (inet_checksum(ip, ihl) != 0) {
        nif->rx_errors++;
        zbuf_free(zb);
        return;
    }

    /* Check destination */
    uint32_t dst = ntohl(ip->dst);
    if (dst != nif->ip && dst != IP4_ADDR_BROADCAST) {
        zbuf_free(zb);
        return;
    }

    /* Set layer offset */
    zb->l4_offset = zb->l3_offset + ihl;

    /* Pull IP header */
    zbuf_pull(zb, ihl);

    /* Dispatch by protocol */
    switch (ip->proto) {
    case IP_PROTO_ICMP:
        icmp_input(nif, zb);
        break;
    case IP_PROTO_UDP:
        udp_input(nif, zb);
        break;
    case IP_PROTO_TCP:
        tcp_input(nif, zb);
        break;
    default:
        zbuf_free(zb);
        break;
    }
}

status_t ip_output(zbuf_t *zb, uint32_t src, uint32_t dst, uint8_t proto)
{
    netif_t *nif = netif_default;
    if (nif == NULL) {
        zbuf_free(zb);
        return STATUS_ERROR;
    }

    if (src == 0) src = nif->ip;

    /* Push IP header */
    ip_hdr_t *ip = (ip_hdr_t *)zbuf_push(zb, sizeof(ip_hdr_t));
    if (ip == NULL) {
        zbuf_free(zb);
        return STATUS_NO_MEM;
    }

    static uint16_t ip_id = 0;

    ip->ver_ihl = 0x45;  /* IPv4, 5 words */
    ip->tos = 0;
    ip->len = htons(zb->len);
    ip->id = htons(ip_id++);
    ip->frag = 0;
    ip->ttl = 64;
    ip->proto = proto;
    ip->checksum = 0;
    ip->src = htonl(src);
    ip->dst = htonl(dst);
    ip->checksum = inet_checksum(ip, sizeof(ip_hdr_t));

    /* Resolve MAC address */
    uint8_t dst_mac[6];
    uint32_t next_hop = dst;

    /* Use gateway for non-local addresses */
    if ((dst & nif->netmask) != (nif->ip & nif->netmask)) {
        next_hop = nif->gateway;
    }

    /* Broadcast */
    if (dst == IP4_ADDR_BROADCAST || (dst & ~nif->netmask) == ~nif->netmask) {
        for (int i = 0; i < 6; i++) dst_mac[i] = 0xFF;
    } else {
        status_t ret = arp_resolve(next_hop, dst_mac);
        if (ret != STATUS_OK) {
            /* Queue packet for ARP resolution */
            zbuf_free(zb);
            return ret;
        }
    }

    return eth_output(nif, zb, dst_mac, ETH_TYPE_IP);
}

/*
 * ICMP Functions
 */
void icmp_input(netif_t *nif __attribute__((unused)), zbuf_t *zb)
{
    if (zb->len < sizeof(icmp_hdr_t)) {
        zbuf_free(zb);
        return;
    }

    icmp_hdr_t *icmp = (icmp_hdr_t *)zb->data;

    if (icmp->type == ICMP_ECHO_REQUEST) {
        /* Get original IP header */
        ip_hdr_t *orig_ip = (ip_hdr_t *)(zb->data - sizeof(ip_hdr_t));
        uint32_t src = ntohl(orig_ip->src);
        uint32_t dst = ntohl(orig_ip->dst);

        /* Modify ICMP header for reply */
        icmp->type = ICMP_ECHO_REPLY;
        icmp->checksum = 0;
        icmp->checksum = inet_checksum(icmp, zb->len);

        /* Send reply */
        ip_output(zb, dst, src, IP_PROTO_ICMP);
    } else {
        zbuf_free(zb);
    }
}

/*
 * UDP Functions
 */
void udp_input(netif_t *nif __attribute__((unused)), zbuf_t *zb)
{
    if (zb->len < sizeof(udp_hdr_t)) {
        zbuf_free(zb);
        return;
    }

    udp_hdr_t *udp = (udp_hdr_t *)zb->data;
    uint16_t dport = ntohs(udp->dport);
    uint16_t sport = ntohs(udp->sport);

    /* Get IP addresses from IP header */
    ip_hdr_t *ip = (ip_hdr_t *)(zb->data - sizeof(ip_hdr_t));
    uint32_t src_ip = ntohl(ip->src);
    (void)src_ip;
    uint32_t dst_ip __attribute__((unused)) = ntohl(ip->dst);

    /* Find matching socket */
    spin_lock(&socket_lock);
    for (int i = 0; i < CONFIG_NET_MAX_SOCKETS; i++) {
        socket_t *sock = socket_table[i];
        if (sock != NULL && sock->type == SOCK_DGRAM) {
            if (sock->local.port == dport) {
                /* Set remote address */
                sock->remote.addr = src_ip;
                sock->remote.port = sport;

                /* Pull UDP header */
                zbuf_pull(zb, UDP_HDR_LEN);

                /* Queue packet */
                zbuf_queue_push(&sock->rx_queue, zb);
                sem_post(&sock->rx_sem);

                spin_unlock(&socket_lock);
                return;
            }
        }
    }
    spin_unlock(&socket_lock);

    zbuf_free(zb);
}

status_t udp_output(zbuf_t *zb, sockaddr_t *src, sockaddr_t *dst)
{
    /* Push UDP header */
    udp_hdr_t *udp = (udp_hdr_t *)zbuf_push(zb, UDP_HDR_LEN);
    if (udp == NULL) {
        zbuf_free(zb);
        return STATUS_NO_MEM;
    }

    udp->sport = htons(src->port);
    udp->dport = htons(dst->port);
    udp->len = htons(zb->len);
    udp->checksum = 0;  /* Optional for UDP over IPv4 */

    return ip_output(zb, src->addr, dst->addr, IP_PROTO_UDP);
}
