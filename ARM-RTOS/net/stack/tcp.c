/*
 * Gracemont Industrial Control Framework
 * TCP Implementation with Zero-Copy Support
 */

#include "net_stack.h"
#include "rtos_config.h"

extern socket_t *socket_table[CONFIG_NET_MAX_SOCKETS];
extern spinlock_t socket_lock;

static socket_t *tcp_listen_list = NULL;
static socket_t *tcp_conn_list = NULL;
static spinlock_t tcp_lock = SPINLOCK_INIT;

/*
 * TCP Checksum Calculation
 */
static uint16_t tcp_checksum(ip_hdr_t *ip, tcp_hdr_t *tcp, uint16_t tcp_len)
{
    uint32_t sum = inet_pseudo_checksum(ntohl(ip->src), ntohl(ip->dst),
                                        IP_PROTO_TCP, tcp_len);

    const uint16_t *ptr = (const uint16_t *)tcp;
    while (tcp_len > 1) {
        sum += *ptr++;
        tcp_len -= 2;
    }
    if (tcp_len == 1) {
        sum += *(const uint8_t *)ptr;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

/*
 * TCP Output
 */
static status_t tcp_send_segment(socket_t *sock, uint8_t flags, zbuf_t *data)
{
    zbuf_t *zb;

    if (data != NULL) {
        zb = data;
    } else {
        zb = zbuf_alloc_tx(0);
        if (zb == NULL) return STATUS_NO_MEM;
    }

    /* Push TCP header */
    tcp_hdr_t *tcp = (tcp_hdr_t *)zbuf_push(zb, sizeof(tcp_hdr_t));
    if (tcp == NULL) {
        if (data == NULL) zbuf_free(zb);
        return STATUS_NO_MEM;
    }

    tcp->sport = htons(sock->local.port);
    tcp->dport = htons(sock->remote.port);
    tcp->seq = htonl(sock->snd_nxt);
    tcp->ack = htonl(sock->rcv_nxt);
    tcp->off_rsvd = (5 << 4);  /* 20 bytes header */
    tcp->flags = flags;
    tcp->win = htons(sock->rcv_wnd);
    tcp->checksum = 0;
    tcp->urgent = 0;

    /* Calculate data length */
    uint16_t data_len = zb->len - sizeof(tcp_hdr_t);

    /* Update sequence number */
    if (flags & (TCP_FLAG_SYN | TCP_FLAG_FIN)) {
        sock->snd_nxt++;
    }
    sock->snd_nxt += data_len;

    return ip_output(zb, sock->local.addr, sock->remote.addr, IP_PROTO_TCP);
}

/*
 * TCP Connection Lookup
 */
static socket_t *tcp_find_socket(uint32_t local_ip, uint16_t local_port,
                                  uint32_t remote_ip, uint16_t remote_port)
{
    socket_t *sock;

    /* First look for exact match (established connections) */
    spin_lock(&tcp_lock);
    for (sock = tcp_conn_list; sock != NULL; sock = sock->next) {
        if (sock->local.port == local_port &&
            sock->remote.port == remote_port &&
            sock->remote.addr == remote_ip) {
            spin_unlock(&tcp_lock);
            return sock;
        }
    }

    /* Then look for listening socket */
    for (sock = tcp_listen_list; sock != NULL; sock = sock->next) {
        if (sock->local.port == local_port) {
            spin_unlock(&tcp_lock);
            return sock;
        }
    }
    spin_unlock(&tcp_lock);

    return NULL;
}

/*
 * TCP Input Handler
 */
void tcp_input(netif_t *nif, zbuf_t *zb)
{
    if (zb->len < sizeof(tcp_hdr_t)) {
        zbuf_free(zb);
        return;
    }

    tcp_hdr_t *tcp = (tcp_hdr_t *)zb->data;
    ip_hdr_t *ip = (ip_hdr_t *)(zb->data - sizeof(ip_hdr_t));

    uint16_t tcp_len = zb->len;
    uint8_t tcp_hdr_len = TCP_HDR_LEN(tcp);

    /* Verify checksum */
    if (tcp_checksum(ip, tcp, tcp_len) != 0) {
        nif->rx_errors++;
        zbuf_free(zb);
        return;
    }

    uint32_t src_ip = ntohl(ip->src);
    uint32_t dst_ip = ntohl(ip->dst);
    uint16_t src_port = ntohs(tcp->sport);
    uint16_t dst_port = ntohs(tcp->dport);
    uint32_t seq = ntohl(tcp->seq);
    uint32_t ack = ntohl(tcp->ack);
    uint8_t flags = tcp->flags;

    /* Find socket */
    socket_t *sock = tcp_find_socket(dst_ip, dst_port, src_ip, src_port);
    if (sock == NULL) {
        /* Send RST for unknown connection */
        if (!(flags & TCP_FLAG_RST)) {
            /* TODO: Send RST */
        }
        zbuf_free(zb);
        return;
    }

    mutex_lock(&sock->lock);

    /* TCP State Machine */
    switch (sock->state) {
    case TCP_LISTEN:
        if (flags & TCP_FLAG_SYN) {
            /* Create new connection socket */
            /* For simplicity, accept on same socket */
            sock->remote.addr = src_ip;
            sock->remote.port = src_port;
            sock->rcv_nxt = seq + 1;
            sock->snd_nxt = get_system_ticks();  /* ISN */
            sock->snd_una = sock->snd_nxt;
            sock->state = TCP_SYN_RECEIVED;

            /* Send SYN-ACK */
            tcp_send_segment(sock, TCP_FLAG_SYN | TCP_FLAG_ACK, NULL);
        }
        break;

    case TCP_SYN_SENT:
        if ((flags & (TCP_FLAG_SYN | TCP_FLAG_ACK)) == (TCP_FLAG_SYN | TCP_FLAG_ACK)) {
            sock->rcv_nxt = seq + 1;
            sock->snd_una = ack;
            sock->state = TCP_ESTABLISHED;

            /* Send ACK */
            tcp_send_segment(sock, TCP_FLAG_ACK, NULL);
            sem_post(&sock->tx_sem);  /* Wake connect() */
        }
        break;

    case TCP_SYN_RECEIVED:
        if (flags & TCP_FLAG_ACK) {
            sock->snd_una = ack;
            sock->state = TCP_ESTABLISHED;
            sem_post(&sock->rx_sem);  /* Wake accept() */
        }
        break;

    case TCP_ESTABLISHED:
        /* Handle incoming data */
        if (flags & TCP_FLAG_ACK) {
            sock->snd_una = ack;
        }

        /* Process data */
        if (tcp_hdr_len < zb->len) {
            zbuf_pull(zb, tcp_hdr_len);

            /* Check sequence number */
            if (seq == sock->rcv_nxt) {
                sock->rcv_nxt += zb->len;

                /* Queue data (zero-copy) */
                zbuf_queue_push(&sock->rx_queue, zb);
                sem_post(&sock->rx_sem);
                zb = NULL;  /* Don't free */

                /* Send ACK */
                tcp_send_segment(sock, TCP_FLAG_ACK, NULL);
            }
        }

        /* Handle FIN */
        if (flags & TCP_FLAG_FIN) {
            sock->rcv_nxt++;
            sock->state = TCP_CLOSE_WAIT;
            tcp_send_segment(sock, TCP_FLAG_ACK, NULL);
            sem_post(&sock->rx_sem);  /* Wake recv() */
        }
        break;

    case TCP_FIN_WAIT_1:
        if (flags & TCP_FLAG_ACK) {
            sock->snd_una = ack;
            if (flags & TCP_FLAG_FIN) {
                sock->rcv_nxt++;
                sock->state = TCP_TIME_WAIT;
                tcp_send_segment(sock, TCP_FLAG_ACK, NULL);
            } else {
                sock->state = TCP_FIN_WAIT_2;
            }
        }
        break;

    case TCP_FIN_WAIT_2:
        if (flags & TCP_FLAG_FIN) {
            sock->rcv_nxt++;
            sock->state = TCP_TIME_WAIT;
            tcp_send_segment(sock, TCP_FLAG_ACK, NULL);
        }
        break;

    case TCP_CLOSE_WAIT:
        /* Application must call close() */
        break;

    case TCP_LAST_ACK:
        if (flags & TCP_FLAG_ACK) {
            sock->state = TCP_CLOSED;
        }
        break;

    case TCP_TIME_WAIT:
        /* Wait for 2*MSL then close */
        break;

    default:
        break;
    }

    mutex_unlock(&sock->lock);

    if (zb != NULL) {
        zbuf_free(zb);
    }
}

status_t tcp_output(socket_t *sock, zbuf_t *zb)
{
    if (sock->state != TCP_ESTABLISHED) {
        zbuf_free(zb);
        return STATUS_ERROR;
    }

    return tcp_send_segment(sock, TCP_FLAG_ACK | TCP_FLAG_PSH, zb);
}

/*
 * Socket API Implementation
 */
int sock_socket(int type)
{
    socket_t *sock = heap_alloc(sizeof(socket_t));
    if (sock == NULL) {
        return -1;
    }

    sock->type = type;
    sock->state = TCP_CLOSED;
    sock->local.addr = 0;
    sock->local.port = 0;
    sock->remote.addr = 0;
    sock->remote.port = 0;
    sock->snd_una = 0;
    sock->snd_nxt = 0;
    sock->snd_wnd = CONFIG_TCP_WINDOW_SIZE;
    sock->rcv_nxt = 0;
    sock->rcv_wnd = CONFIG_TCP_WINDOW_SIZE;
    sock->flags = 0;
    sock->timeout = 0;

    zbuf_queue_init(&sock->rx_queue);
    zbuf_queue_init(&sock->tx_queue);
    sem_init(&sock->rx_sem, 0);
    sem_init(&sock->tx_sem, 0);
    mutex_init(&sock->lock);

    spin_lock_irq(&socket_lock);
    int fd = next_fd++;
    sock->fd = fd;
    socket_table[fd % CONFIG_NET_MAX_SOCKETS] = sock;
    spin_unlock_irq(&socket_lock);

    return fd;
}

int sock_bind(int fd, sockaddr_t *addr)
{
    socket_t *sock = socket_table[fd % CONFIG_NET_MAX_SOCKETS];
    if (sock == NULL) return -1;

    mutex_lock(&sock->lock);
    sock->local = *addr;
    mutex_unlock(&sock->lock);

    return 0;
}

int sock_listen(int fd, int backlog)
{
    socket_t *sock = socket_table[fd % CONFIG_NET_MAX_SOCKETS];
    if (sock == NULL || sock->type != SOCK_STREAM) return -1;

    mutex_lock(&sock->lock);
    sock->state = TCP_LISTEN;
    mutex_unlock(&sock->lock);

    /* Add to listen list */
    spin_lock_irq(&tcp_lock);
    sock->next = tcp_listen_list;
    tcp_listen_list = sock;
    spin_unlock_irq(&tcp_lock);

    return 0;
}

int sock_accept(int fd, sockaddr_t *addr)
{
    socket_t *sock = socket_table[fd % CONFIG_NET_MAX_SOCKETS];
    if (sock == NULL || sock->state != TCP_LISTEN) return -1;

    /* Wait for connection */
    sem_wait(&sock->rx_sem);

    if (addr != NULL) {
        *addr = sock->remote;
    }

    return fd;  /* Simplified: return same fd */
}

int sock_connect(int fd, sockaddr_t *addr)
{
    socket_t *sock = socket_table[fd % CONFIG_NET_MAX_SOCKETS];
    if (sock == NULL) return -1;

    mutex_lock(&sock->lock);

    sock->remote = *addr;
    if (sock->local.port == 0) {
        sock->local.port = 49152 + (get_system_ticks() % 16384);
    }
    if (sock->local.addr == 0) {
        netif_t *nif = netif_get_default();
        if (nif) sock->local.addr = nif->ip;
    }

    sock->snd_nxt = get_system_ticks();  /* ISN */
    sock->snd_una = sock->snd_nxt;
    sock->state = TCP_SYN_SENT;

    /* Send SYN */
    tcp_send_segment(sock, TCP_FLAG_SYN, NULL);
    mutex_unlock(&sock->lock);

    /* Add to connection list */
    spin_lock_irq(&tcp_lock);
    sock->next = tcp_conn_list;
    tcp_conn_list = sock;
    spin_unlock_irq(&tcp_lock);

    /* Wait for connection */
    sem_wait(&sock->tx_sem);

    return (sock->state == TCP_ESTABLISHED) ? 0 : -1;
}

int sock_send(int fd, const void *data, size_t len)
{
    socket_t *sock = socket_table[fd % CONFIG_NET_MAX_SOCKETS];
    if (sock == NULL) return -1;

    zbuf_t *zb = zbuf_alloc_tx(len);
    if (zb == NULL) return -1;

    /* Copy data to buffer */
    uint8_t *dst = zbuf_put(zb, len);
    const uint8_t *src = (const uint8_t *)data;
    for (size_t i = 0; i < len; i++) {
        dst[i] = src[i];
    }

    status_t ret = tcp_output(sock, zb);
    return (ret == STATUS_OK) ? (int)len : -1;
}

int sock_recv(int fd, void *data, size_t len)
{
    socket_t *sock = socket_table[fd % CONFIG_NET_MAX_SOCKETS];
    if (sock == NULL) return -1;

    /* Wait for data */
    sem_wait(&sock->rx_sem);

    /* Get buffer from queue */
    zbuf_t *zb = zbuf_queue_pop(&sock->rx_queue);
    if (zb == NULL) {
        if (sock->state != TCP_ESTABLISHED) {
            return 0;  /* Connection closed */
        }
        return -1;
    }

    /* Copy data */
    size_t copy_len = (zb->len < len) ? zb->len : len;
    uint8_t *dst = (uint8_t *)data;
    uint8_t *src = zb->data;
    for (size_t i = 0; i < copy_len; i++) {
        dst[i] = src[i];
    }

    zbuf_free(zb);
    return (int)copy_len;
}

/*
 * Zero-Copy Socket API
 */
zbuf_t *sock_recv_zbuf(int fd)
{
    socket_t *sock = socket_table[fd % CONFIG_NET_MAX_SOCKETS];
    if (sock == NULL) return NULL;

    sem_wait(&sock->rx_sem);
    return zbuf_queue_pop(&sock->rx_queue);
}

int sock_send_zbuf(int fd, zbuf_t *zb)
{
    socket_t *sock = socket_table[fd % CONFIG_NET_MAX_SOCKETS];
    if (sock == NULL) {
        zbuf_free(zb);
        return -1;
    }

    status_t ret = tcp_output(sock, zb);
    return (ret == STATUS_OK) ? 0 : -1;
}

int sock_sendto(int fd, const void *data, size_t len, sockaddr_t *dst)
{
    socket_t *sock = socket_table[fd % CONFIG_NET_MAX_SOCKETS];
    if (sock == NULL || sock->type != SOCK_DGRAM) return -1;

    zbuf_t *zb = zbuf_alloc_tx(len);
    if (zb == NULL) return -1;

    uint8_t *buf = zbuf_put(zb, len);
    const uint8_t *src = (const uint8_t *)data;
    for (size_t i = 0; i < len; i++) {
        buf[i] = src[i];
    }

    status_t ret = udp_output(zb, &sock->local, dst);
    return (ret == STATUS_OK) ? (int)len : -1;
}

int sock_recvfrom(int fd, void *data, size_t len, sockaddr_t *src)
{
    socket_t *sock = socket_table[fd % CONFIG_NET_MAX_SOCKETS];
    if (sock == NULL || sock->type != SOCK_DGRAM) return -1;

    sem_wait(&sock->rx_sem);

    zbuf_t *zb = zbuf_queue_pop(&sock->rx_queue);
    if (zb == NULL) return -1;

    if (src != NULL) {
        *src = sock->remote;
    }

    size_t copy_len = (zb->len < len) ? zb->len : len;
    uint8_t *dst = (uint8_t *)data;
    uint8_t *buf = zb->data;
    for (size_t i = 0; i < copy_len; i++) {
        dst[i] = buf[i];
    }

    zbuf_free(zb);
    return (int)copy_len;
}

int sock_close(int fd)
{
    socket_t *sock = socket_table[fd % CONFIG_NET_MAX_SOCKETS];
    if (sock == NULL) return -1;

    mutex_lock(&sock->lock);

    if (sock->type == SOCK_STREAM && sock->state == TCP_ESTABLISHED) {
        sock->state = TCP_FIN_WAIT_1;
        tcp_send_segment(sock, TCP_FLAG_FIN | TCP_FLAG_ACK, NULL);
    }

    mutex_unlock(&sock->lock);

    /* Flush queues */
    zbuf_queue_flush(&sock->rx_queue);
    zbuf_queue_flush(&sock->tx_queue);

    spin_lock_irq(&socket_lock);
    socket_table[fd % CONFIG_NET_MAX_SOCKETS] = NULL;
    spin_unlock_irq(&socket_lock);

    heap_free(sock);
    return 0;
}
