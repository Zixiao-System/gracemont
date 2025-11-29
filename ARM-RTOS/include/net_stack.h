/*
 * Gracemont Industrial Control Framework
 * Lightweight TCP/IP Stack - Header Definitions
 */

#ifndef NET_STACK_H
#define NET_STACK_H

#include "rtos_types.h"
#include "zbuf.h"

/* Ethernet Header */
typedef struct PACKED {
    uint8_t     dst[6];
    uint8_t     src[6];
    uint16_t    type;
} eth_hdr_t;

#define ETH_HDR_LEN     14
#define ETH_TYPE_IP     0x0800
#define ETH_TYPE_ARP    0x0806
#define ETH_TYPE_VLAN   0x8100
#define ETH_TYPE_PNIO   0x8892

/* ARP Header */
typedef struct PACKED {
    uint16_t    htype;
    uint16_t    ptype;
    uint8_t     hlen;
    uint8_t     plen;
    uint16_t    oper;
    uint8_t     sha[6];
    uint32_t    spa;
    uint8_t     tha[6];
    uint32_t    tpa;
} arp_hdr_t;

#define ARP_OP_REQUEST  1
#define ARP_OP_REPLY    2

/* IP Header */
typedef struct PACKED {
    uint8_t     ver_ihl;
    uint8_t     tos;
    uint16_t    len;
    uint16_t    id;
    uint16_t    frag;
    uint8_t     ttl;
    uint8_t     proto;
    uint16_t    checksum;
    uint32_t    src;
    uint32_t    dst;
} ip_hdr_t;

#define IP_HDR_LEN(iph)     (((iph)->ver_ihl & 0x0F) << 2)
#define IP_PROTO_ICMP       1
#define IP_PROTO_TCP        6
#define IP_PROTO_UDP        17

/* ICMP Header */
typedef struct PACKED {
    uint8_t     type;
    uint8_t     code;
    uint16_t    checksum;
    uint16_t    id;
    uint16_t    seq;
} icmp_hdr_t;

#define ICMP_ECHO_REQUEST   8
#define ICMP_ECHO_REPLY     0

/* UDP Header */
typedef struct PACKED {
    uint16_t    sport;
    uint16_t    dport;
    uint16_t    len;
    uint16_t    checksum;
} udp_hdr_t;

#define UDP_HDR_LEN     8

/* TCP Header */
typedef struct PACKED {
    uint16_t    sport;
    uint16_t    dport;
    uint32_t    seq;
    uint32_t    ack;
    uint8_t     off_rsvd;
    uint8_t     flags;
    uint16_t    win;
    uint16_t    checksum;
    uint16_t    urgent;
} tcp_hdr_t;

#define TCP_HDR_LEN(tcph)   ((((tcph)->off_rsvd >> 4) & 0x0F) << 2)
#define TCP_FLAG_FIN        0x01
#define TCP_FLAG_SYN        0x02
#define TCP_FLAG_RST        0x04
#define TCP_FLAG_PSH        0x08
#define TCP_FLAG_ACK        0x10
#define TCP_FLAG_URG        0x20

/* Network Interface */
typedef struct netif {
    char            name[8];
    uint8_t         mac[6];
    uint32_t        ip;
    uint32_t        netmask;
    uint32_t        gateway;
    uint16_t        mtu;
    bool            up;
    void            *priv;

    /* Statistics */
    uint64_t        rx_packets;
    uint64_t        rx_bytes;
    uint64_t        tx_packets;
    uint64_t        tx_bytes;
    uint64_t        rx_errors;
    uint64_t        tx_errors;

    /* Driver callbacks */
    status_t        (*send)(struct netif *nif, zbuf_t *zb);
    status_t        (*ioctl)(struct netif *nif, int cmd, void *arg);

    struct netif    *next;
} netif_t;

/* Socket Address */
typedef struct {
    uint32_t    addr;
    uint16_t    port;
} sockaddr_t;

/* Socket Types */
#define SOCK_STREAM     1   /* TCP */
#define SOCK_DGRAM      2   /* UDP */
#define SOCK_RAW        3   /* Raw IP */

/* Socket States (TCP) */
typedef enum {
    TCP_CLOSED = 0,
    TCP_LISTEN,
    TCP_SYN_SENT,
    TCP_SYN_RECEIVED,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_CLOSE_WAIT,
    TCP_CLOSING,
    TCP_LAST_ACK,
    TCP_TIME_WAIT
} tcp_state_t;

/* Socket Control Block */
typedef struct socket {
    int             fd;
    int             type;
    int             state;

    sockaddr_t      local;
    sockaddr_t      remote;

    /* TCP specific */
    uint32_t        snd_una;    /* Unacknowledged */
    uint32_t        snd_nxt;    /* Next to send */
    uint32_t        snd_wnd;    /* Send window */
    uint32_t        rcv_nxt;    /* Next expected */
    uint32_t        rcv_wnd;    /* Receive window */

    /* Buffers */
    zbuf_queue_t    rx_queue;
    zbuf_queue_t    tx_queue;

    /* Synchronization */
    semaphore_t     rx_sem;
    semaphore_t     tx_sem;
    mutex_t         lock;

    /* Options */
    uint32_t        flags;
    tick_t          timeout;

    /* Linked list */
    struct socket   *next;
} socket_t;

/* API Functions */

/* Network Interface */
status_t netif_register(netif_t *nif);
status_t netif_unregister(netif_t *nif);
netif_t *netif_get_default(void);
void netif_input(netif_t *nif, zbuf_t *zb);

/* IP Layer */
status_t ip_output(zbuf_t *zb, uint32_t src, uint32_t dst, uint8_t proto);
void ip_input(netif_t *nif, zbuf_t *zb);

/* ARP */
status_t arp_resolve(uint32_t ip, uint8_t *mac);
void arp_input(netif_t *nif, zbuf_t *zb);

/* ICMP */
void icmp_input(netif_t *nif, zbuf_t *zb);

/* UDP */
status_t udp_output(zbuf_t *zb, sockaddr_t *src, sockaddr_t *dst);
void udp_input(netif_t *nif, zbuf_t *zb);

/* TCP */
status_t tcp_output(socket_t *sock, zbuf_t *zb);
void tcp_input(netif_t *nif, zbuf_t *zb);
void tcp_timer(void);

/* Socket API */
int sock_socket(int type);
int sock_bind(int fd, sockaddr_t *addr);
int sock_listen(int fd, int backlog);
int sock_accept(int fd, sockaddr_t *addr);
int sock_connect(int fd, sockaddr_t *addr);
int sock_send(int fd, const void *data, size_t len);
int sock_recv(int fd, void *data, size_t len);
int sock_sendto(int fd, const void *data, size_t len, sockaddr_t *dst);
int sock_recvfrom(int fd, void *data, size_t len, sockaddr_t *src);
int sock_close(int fd);

/* Zero-copy socket API */
zbuf_t *sock_recv_zbuf(int fd);
int sock_send_zbuf(int fd, zbuf_t *zb);

/* Utilities */
uint16_t inet_checksum(const void *data, size_t len);
uint16_t inet_pseudo_checksum(uint32_t src, uint32_t dst, uint8_t proto, uint16_t len);
uint32_t htonl(uint32_t h);
uint16_t htons(uint16_t h);
uint32_t ntohl(uint32_t n);
uint16_t ntohs(uint16_t n);

#define HTONL(x)    htonl(x)
#define HTONS(x)    htons(x)
#define NTOHL(x)    ntohl(x)
#define NTOHS(x)    ntohs(x)

/* IP Address Macros */
#define IP4_ADDR(a,b,c,d)   (((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | \
                             ((uint32_t)(c) << 8) | (uint32_t)(d))
#define IP4_ADDR_ANY        0x00000000
#define IP4_ADDR_BROADCAST  0xFFFFFFFF

/* Stack Initialization */
void net_stack_init(void);
void net_stack_poll(void);

#endif /* NET_STACK_H */
