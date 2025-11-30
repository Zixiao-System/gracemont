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
 * Virtio Network Driver for ARM64 (QEMU virt platform)
 */

#include "eth.h"
#include "rtos_config.h"
#include "rtos.h"
#include "zbuf.h"
#include "net_stack.h"

/*
 * Virtio MMIO Registers
 */
#define VIRTIO_MMIO_MAGIC           0x000
#define VIRTIO_MMIO_VERSION         0x004
#define VIRTIO_MMIO_DEVICE_ID       0x008
#define VIRTIO_MMIO_VENDOR_ID       0x00c
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024
#define VIRTIO_MMIO_QUEUE_SEL       0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX   0x034
#define VIRTIO_MMIO_QUEUE_NUM       0x038
#define VIRTIO_MMIO_QUEUE_READY     0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY    0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060
#define VIRTIO_MMIO_INTERRUPT_ACK   0x064
#define VIRTIO_MMIO_STATUS          0x070
#define VIRTIO_MMIO_QUEUE_DESC_LOW  0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x084
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW 0x090
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH 0x094
#define VIRTIO_MMIO_QUEUE_USED_LOW  0x0a0
#define VIRTIO_MMIO_QUEUE_USED_HIGH 0x0a4
#define VIRTIO_MMIO_CONFIG          0x100

/*
 * Virtio Status Bits
 */
#define VIRTIO_STATUS_ACK           1
#define VIRTIO_STATUS_DRIVER        2
#define VIRTIO_STATUS_DRIVER_OK     4
#define VIRTIO_STATUS_FEATURES_OK   8
#define VIRTIO_STATUS_FAILED        128

/*
 * Virtio Device IDs
 */
#define VIRTIO_DEV_NET              1
#define VIRTIO_DEV_BLK              2

/*
 * Virtio Net Features
 */
#define VIRTIO_NET_F_CSUM           (1 << 0)
#define VIRTIO_NET_F_MAC            (1 << 5)
#define VIRTIO_NET_F_STATUS         (1 << 16)
#define VIRTIO_NET_F_MRG_RXBUF      (1 << 15)

/*
 * Virtio Ring Descriptor Flags
 */
#define VRING_DESC_F_NEXT           1
#define VRING_DESC_F_WRITE          2
#define VRING_DESC_F_INDIRECT       4

/*
 * Queue Sizes
 */
#define VIRTQ_RX                    0
#define VIRTQ_TX                    1
#define VIRTQ_SIZE                  256
#define VIRTQ_ALIGN                 4096

/*
 * Virtio Ring Structures
 */
typedef struct {
    uint64_t    addr;
    uint32_t    len;
    uint16_t    flags;
    uint16_t    next;
} PACKED vring_desc_t;

typedef struct {
    uint16_t    flags;
    uint16_t    idx;
    uint16_t    ring[];
} PACKED vring_avail_t;

typedef struct {
    uint32_t    id;
    uint32_t    len;
} PACKED vring_used_elem_t;

typedef struct {
    uint16_t    flags;
    uint16_t    idx;
    vring_used_elem_t ring[];
} PACKED vring_used_t;

/*
 * Virtio Net Header
 */
typedef struct {
    uint8_t     flags;
    uint8_t     gso_type;
    uint16_t    hdr_len;
    uint16_t    gso_size;
    uint16_t    csum_start;
    uint16_t    csum_offset;
    uint16_t    num_buffers;
} PACKED virtio_net_hdr_t;

#define VIRTIO_NET_HDR_SIZE     12

/*
 * Virtqueue
 */
typedef struct {
    uint16_t        num;
    uint16_t        free_head;
    uint16_t        num_free;
    uint16_t        last_used_idx;

    vring_desc_t    *desc;
    vring_avail_t   *avail;
    vring_used_t    *used;

    /* Buffer tracking */
    zbuf_t          *buffers[VIRTQ_SIZE];

    spinlock_t      lock;
} virtqueue_t;

/*
 * Ethernet Device Context
 */
typedef struct {
    addr_t          base;
    uint32_t        irq;

    /* MAC Address */
    uint8_t         mac[6];

    /* Virtqueues */
    virtqueue_t     rxq;
    virtqueue_t     txq;

    /* DMA Memory */
    void            *dma_mem;
    addr_t          dma_phys;
    size_t          dma_size;

    /* Network Interface */
    netif_t         netif;

    /* Statistics */
    uint64_t        rx_packets;
    uint64_t        tx_packets;
    uint64_t        rx_bytes;
    uint64_t        tx_bytes;
    uint64_t        rx_errors;
    uint64_t        tx_errors;
    uint64_t        rx_dropped;

    spinlock_t      lock;
    bool            initialized;
} eth_dev_t;

/* Device instance */
static eth_dev_t eth_device;

/*
 * MMIO Access
 */
#define VIRTIO_REG(dev, off)    (*(volatile uint32_t *)((dev)->base + (off)))

/*
 * Initialize Virtqueue
 */
static status_t virtq_init(eth_dev_t *dev, virtqueue_t *vq, uint16_t qsel)
{
    /* Select queue */
    VIRTIO_REG(dev, VIRTIO_MMIO_QUEUE_SEL) = qsel;

    /* Get max size */
    uint32_t max = VIRTIO_REG(dev, VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (max == 0) return STATUS_ERROR;
    if (max > VIRTQ_SIZE) max = VIRTQ_SIZE;

    vq->num = max;
    vq->free_head = 0;
    vq->num_free = max;
    vq->last_used_idx = 0;
    vq->lock = (spinlock_t)SPINLOCK_INIT;

    /* Calculate sizes */
    size_t desc_size = sizeof(vring_desc_t) * max;
    size_t avail_size = sizeof(uint16_t) * (3 + max);
    size_t used_size = sizeof(uint16_t) * 3 + sizeof(vring_used_elem_t) * max;

    /* Align sizes */
    desc_size = (desc_size + VIRTQ_ALIGN - 1) & ~(VIRTQ_ALIGN - 1);
    avail_size = (avail_size + VIRTQ_ALIGN - 1) & ~(VIRTQ_ALIGN - 1);
    used_size = (used_size + VIRTQ_ALIGN - 1) & ~(VIRTQ_ALIGN - 1);

    /* Allocate DMA memory */
    size_t total = desc_size + avail_size + used_size;
    void *mem = dma_alloc(total);
    if (!mem) return STATUS_NO_MEM;

    /* Clear memory */
    for (size_t i = 0; i < total; i++) {
        ((uint8_t *)mem)[i] = 0;
    }

    /* Setup pointers */
    vq->desc = (vring_desc_t *)mem;
    vq->avail = (vring_avail_t *)((uint8_t *)mem + desc_size);
    vq->used = (vring_used_t *)((uint8_t *)mem + desc_size + avail_size);

    /* Initialize free list */
    for (uint16_t i = 0; i < max - 1; i++) {
        vq->desc[i].next = i + 1;
    }
    vq->desc[max - 1].next = 0xFFFF;

    /* Configure queue */
    VIRTIO_REG(dev, VIRTIO_MMIO_QUEUE_NUM) = max;

    addr_t phys = (addr_t)mem;  /* Identity mapping */
    VIRTIO_REG(dev, VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint32_t)phys;
    VIRTIO_REG(dev, VIRTIO_MMIO_QUEUE_DESC_HIGH) = (uint32_t)(phys >> 32);

    phys = (addr_t)vq->avail;
    VIRTIO_REG(dev, VIRTIO_MMIO_QUEUE_AVAIL_LOW) = (uint32_t)phys;
    VIRTIO_REG(dev, VIRTIO_MMIO_QUEUE_AVAIL_HIGH) = (uint32_t)(phys >> 32);

    phys = (addr_t)vq->used;
    VIRTIO_REG(dev, VIRTIO_MMIO_QUEUE_USED_LOW) = (uint32_t)phys;
    VIRTIO_REG(dev, VIRTIO_MMIO_QUEUE_USED_HIGH) = (uint32_t)(phys >> 32);

    /* Enable queue */
    VIRTIO_REG(dev, VIRTIO_MMIO_QUEUE_READY) = 1;

    return STATUS_OK;
}

/*
 * Allocate descriptor
 */
static int virtq_alloc_desc(virtqueue_t *vq)
{
    if (vq->num_free == 0) return -1;

    uint16_t idx = vq->free_head;
    vq->free_head = vq->desc[idx].next;
    vq->num_free--;

    return idx;
}

/*
 * Free descriptor
 */
static void virtq_free_desc(virtqueue_t *vq, uint16_t idx)
{
    vq->desc[idx].next = vq->free_head;
    vq->free_head = idx;
    vq->num_free++;
}

/*
 * Add buffer to RX queue
 */
static status_t eth_rx_add_buffer(eth_dev_t *dev)
{
    virtqueue_t *vq = &dev->rxq;

    spin_lock(&vq->lock);

    if (vq->num_free < 2) {
        spin_unlock(&vq->lock);
        return STATUS_NO_MEM;
    }

    /* Allocate zbuf */
    zbuf_t *zb = zbuf_alloc_rx(CONFIG_ZBUF_SIZE);
    if (!zb) {
        spin_unlock(&vq->lock);
        return STATUS_NO_MEM;
    }

    /* Reserve space for virtio header */
    zbuf_reserve(zb, VIRTIO_NET_HDR_SIZE);

    /* Allocate descriptors */
    int hdr_idx = virtq_alloc_desc(vq);
    int data_idx = virtq_alloc_desc(vq);

    if (hdr_idx < 0 || data_idx < 0) {
        if (hdr_idx >= 0) virtq_free_desc(vq, hdr_idx);
        zbuf_free(zb);
        spin_unlock(&vq->lock);
        return STATUS_NO_MEM;
    }

    /* Setup header descriptor */
    vq->desc[hdr_idx].addr = zb->dma_addr;
    vq->desc[hdr_idx].len = VIRTIO_NET_HDR_SIZE;
    vq->desc[hdr_idx].flags = VRING_DESC_F_WRITE | VRING_DESC_F_NEXT;
    vq->desc[hdr_idx].next = data_idx;

    /* Setup data descriptor */
    vq->desc[data_idx].addr = zb->dma_addr + VIRTIO_NET_HDR_SIZE;
    vq->desc[data_idx].len = CONFIG_ZBUF_SIZE - VIRTIO_NET_HDR_SIZE - CONFIG_ZBUF_HEADROOM;
    vq->desc[data_idx].flags = VRING_DESC_F_WRITE;
    vq->desc[data_idx].next = 0;

    /* Save buffer reference */
    vq->buffers[hdr_idx] = zb;

    /* Add to available ring */
    uint16_t avail_idx = vq->avail->idx % vq->num;
    vq->avail->ring[avail_idx] = hdr_idx;

    /* Memory barrier */
    dmb();

    vq->avail->idx++;

    spin_unlock(&vq->lock);

    /* Notify device */
    VIRTIO_REG(dev, VIRTIO_MMIO_QUEUE_NOTIFY) = VIRTQ_RX;

    return STATUS_OK;
}

/*
 * Fill RX queue with buffers
 */
static void eth_rx_fill(eth_dev_t *dev)
{
    while (eth_rx_add_buffer(dev) == STATUS_OK);
}

/*
 * Process received packets
 */
static void eth_rx_process(eth_dev_t *dev)
{
    virtqueue_t *vq = &dev->rxq;

    spin_lock(&vq->lock);

    while (vq->last_used_idx != vq->used->idx) {
        /* Memory barrier */
        dmb();

        uint16_t used_idx = vq->last_used_idx % vq->num;
        uint16_t desc_idx = vq->used->ring[used_idx].id;
        uint32_t len = vq->used->ring[used_idx].len;

        /* Get buffer */
        zbuf_t *zb = vq->buffers[desc_idx];
        vq->buffers[desc_idx] = NULL;

        /* Free descriptors */
        uint16_t next_idx = vq->desc[desc_idx].next;
        virtq_free_desc(vq, desc_idx);
        virtq_free_desc(vq, next_idx);

        vq->last_used_idx++;

        spin_unlock(&vq->lock);

        if (zb && len > VIRTIO_NET_HDR_SIZE) {
            /* Adjust buffer pointers */
            zb->data = zb->head + VIRTIO_NET_HDR_SIZE;
            zb->len = len - VIRTIO_NET_HDR_SIZE;
            zb->tail = zb->data + zb->len;

            /* Update statistics */
            dev->rx_packets++;
            dev->rx_bytes += zb->len;

            /* Pass to network stack */
            netif_input(&dev->netif, zb);
        } else {
            if (zb) zbuf_free(zb);
            dev->rx_errors++;
        }

        /* Refill RX buffer */
        eth_rx_add_buffer(dev);

        spin_lock(&vq->lock);
    }

    spin_unlock(&vq->lock);
}

/*
 * Transmit packet
 */
static status_t eth_send(netif_t *nif, zbuf_t *zb)
{
    eth_dev_t *dev = (eth_dev_t *)nif->priv;
    virtqueue_t *vq = &dev->txq;

    if (!zb || zb->len == 0) {
        if (zb) zbuf_free(zb);
        return STATUS_INVALID;
    }

    spin_lock(&vq->lock);

    if (vq->num_free < 2) {
        spin_unlock(&vq->lock);
        zbuf_free(zb);
        dev->tx_errors++;
        return STATUS_NO_MEM;
    }

    /* Add virtio header */
    uint8_t *hdr = zbuf_push(zb, VIRTIO_NET_HDR_SIZE);
    if (!hdr) {
        spin_unlock(&vq->lock);
        zbuf_free(zb);
        dev->tx_errors++;
        return STATUS_NO_MEM;
    }

    /* Clear header */
    for (int i = 0; i < VIRTIO_NET_HDR_SIZE; i++) {
        hdr[i] = 0;
    }

    /* Allocate descriptors */
    int desc_idx = virtq_alloc_desc(vq);
    if (desc_idx < 0) {
        spin_unlock(&vq->lock);
        zbuf_free(zb);
        dev->tx_errors++;
        return STATUS_NO_MEM;
    }

    /* Setup descriptor */
    vq->desc[desc_idx].addr = zb->dma_addr + (zb->data - zb->head);
    vq->desc[desc_idx].len = zb->len;
    vq->desc[desc_idx].flags = 0;
    vq->desc[desc_idx].next = 0;

    /* Save buffer reference */
    vq->buffers[desc_idx] = zb;

    /* Add to available ring */
    uint16_t avail_idx = vq->avail->idx % vq->num;
    vq->avail->ring[avail_idx] = desc_idx;

    /* Memory barrier */
    dmb();

    vq->avail->idx++;

    /* Update statistics */
    dev->tx_packets++;
    dev->tx_bytes += zb->len - VIRTIO_NET_HDR_SIZE;

    spin_unlock(&vq->lock);

    /* Notify device */
    VIRTIO_REG(dev, VIRTIO_MMIO_QUEUE_NOTIFY) = VIRTQ_TX;

    return STATUS_OK;
}

/*
 * Process transmitted packets
 */
static void eth_tx_process(eth_dev_t *dev)
{
    virtqueue_t *vq = &dev->txq;

    spin_lock(&vq->lock);

    while (vq->last_used_idx != vq->used->idx) {
        dmb();

        uint16_t used_idx = vq->last_used_idx % vq->num;
        uint16_t desc_idx = vq->used->ring[used_idx].id;

        /* Free buffer */
        zbuf_t *zb = vq->buffers[desc_idx];
        vq->buffers[desc_idx] = NULL;
        if (zb) zbuf_free(zb);

        /* Free descriptor */
        virtq_free_desc(vq, desc_idx);

        vq->last_used_idx++;
    }

    spin_unlock(&vq->lock);
}

/*
 * IRQ Handler
 */
static void eth_irq_handler(uint32_t irq __attribute__((unused)), void *arg)
{
    eth_dev_t *dev = (eth_dev_t *)arg;

    uint32_t status = VIRTIO_REG(dev, VIRTIO_MMIO_INTERRUPT_STATUS);
    VIRTIO_REG(dev, VIRTIO_MMIO_INTERRUPT_ACK) = status;

    if (status & 1) {
        /* Used buffer notification */
        eth_rx_process(dev);
        eth_tx_process(dev);
    }
}

/*
 * Initialize Ethernet Driver
 */
status_t eth_init(void)
{
    eth_dev_t *dev = &eth_device;

    dev->base = CONFIG_ETH_BASE;
    dev->irq = CONFIG_ETH_IRQ;
    dev->lock = (spinlock_t)SPINLOCK_INIT;

    /* Check magic */
    if (VIRTIO_REG(dev, VIRTIO_MMIO_MAGIC) != 0x74726976) {
        return STATUS_ERROR;
    }

    /* Check device type */
    if (VIRTIO_REG(dev, VIRTIO_MMIO_DEVICE_ID) != VIRTIO_DEV_NET) {
        return STATUS_ERROR;
    }

    /* Reset device */
    VIRTIO_REG(dev, VIRTIO_MMIO_STATUS) = 0;

    /* Acknowledge */
    VIRTIO_REG(dev, VIRTIO_MMIO_STATUS) = VIRTIO_STATUS_ACK;

    /* Driver */
    VIRTIO_REG(dev, VIRTIO_MMIO_STATUS) = VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER;

    /* Negotiate features */
    uint32_t features = VIRTIO_REG(dev, VIRTIO_MMIO_DEVICE_FEATURES);
    features &= VIRTIO_NET_F_MAC;  /* Only use MAC feature */
    VIRTIO_REG(dev, VIRTIO_MMIO_DRIVER_FEATURES) = features;

    /* Features OK */
    VIRTIO_REG(dev, VIRTIO_MMIO_STATUS) = VIRTIO_STATUS_ACK |
                                           VIRTIO_STATUS_DRIVER |
                                           VIRTIO_STATUS_FEATURES_OK;

    /* Check features accepted */
    if (!(VIRTIO_REG(dev, VIRTIO_MMIO_STATUS) & VIRTIO_STATUS_FEATURES_OK)) {
        VIRTIO_REG(dev, VIRTIO_MMIO_STATUS) = VIRTIO_STATUS_FAILED;
        return STATUS_ERROR;
    }

    /* Read MAC address */
    if (features & VIRTIO_NET_F_MAC) {
        volatile uint8_t *config = (volatile uint8_t *)(dev->base + VIRTIO_MMIO_CONFIG);
        for (int i = 0; i < 6; i++) {
            dev->mac[i] = config[i];
        }
    } else {
        /* Default MAC */
        dev->mac[0] = 0x52;
        dev->mac[1] = 0x54;
        dev->mac[2] = 0x00;
        dev->mac[3] = 0x12;
        dev->mac[4] = 0x34;
        dev->mac[5] = 0x56;
    }

    /* Initialize virtqueues */
    if (virtq_init(dev, &dev->rxq, VIRTQ_RX) != STATUS_OK) {
        VIRTIO_REG(dev, VIRTIO_MMIO_STATUS) = VIRTIO_STATUS_FAILED;
        return STATUS_ERROR;
    }

    if (virtq_init(dev, &dev->txq, VIRTQ_TX) != STATUS_OK) {
        VIRTIO_REG(dev, VIRTIO_MMIO_STATUS) = VIRTIO_STATUS_FAILED;
        return STATUS_ERROR;
    }

    /* Driver OK */
    VIRTIO_REG(dev, VIRTIO_MMIO_STATUS) = VIRTIO_STATUS_ACK |
                                           VIRTIO_STATUS_DRIVER |
                                           VIRTIO_STATUS_FEATURES_OK |
                                           VIRTIO_STATUS_DRIVER_OK;

    /* Setup network interface */
    dev->netif.name[0] = 'e'; dev->netif.name[1] = 't'; dev->netif.name[2] = 'h';
    dev->netif.name[3] = '0'; dev->netif.name[4] = '\0';

    for (int i = 0; i < 6; i++) {
        dev->netif.mac[i] = dev->mac[i];
    }

    dev->netif.mtu = 1500;
    dev->netif.up = true;
    dev->netif.send = eth_send;
    dev->netif.priv = dev;

    /* Register IRQ handler */
    irq_register(dev->irq, eth_irq_handler, dev);
    irq_enable(dev->irq);

    /* Fill RX queue */
    eth_rx_fill(dev);

    /* Register network interface */
    netif_register(&dev->netif);

    dev->initialized = true;

    return STATUS_OK;
}

/*
 * Get network interface
 */
netif_t *eth_get_netif(void)
{
    if (!eth_device.initialized) return NULL;
    return &eth_device.netif;
}

/*
 * Get statistics
 */
void eth_get_stats(eth_stats_t *stats)
{
    if (!stats) return;

    eth_dev_t *dev = &eth_device;
    stats->rx_packets = dev->rx_packets;
    stats->tx_packets = dev->tx_packets;
    stats->rx_bytes = dev->rx_bytes;
    stats->tx_bytes = dev->tx_bytes;
    stats->rx_errors = dev->rx_errors;
    stats->tx_errors = dev->tx_errors;
    stats->rx_dropped = dev->rx_dropped;
}

/*
 * Poll for packets (for non-interrupt mode)
 */
void eth_poll(void)
{
    if (!eth_device.initialized) return;

    eth_rx_process(&eth_device);
    eth_tx_process(&eth_device);
}
