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
 * Zero-Copy Network Buffer Management
 */

#ifndef ZBUF_H
#define ZBUF_H

#include "rtos_types.h"
#include "rtos_config.h"

/*
 * Zero-Copy Buffer Structure
 *
 * Layout:
 * +------------------+
 * | zbuf_t header    |
 * +------------------+
 * | headroom         | <- CONFIG_ZBUF_HEADROOM bytes
 * +------------------+
 * | data area        | <- data pointer points here
 * +------------------+
 * | tailroom         |
 * +------------------+
 *
 * The buffer supports push/pull operations for adding/removing
 * protocol headers without copying data.
 */

typedef struct zbuf {
    /* Buffer pointers */
    uint8_t         *head;          /* Start of buffer space */
    uint8_t         *data;          /* Start of actual data */
    uint8_t         *tail;          /* End of actual data */
    uint8_t         *end;           /* End of buffer space */

    /* Buffer info */
    uint16_t        len;            /* Data length */
    uint16_t        size;           /* Total buffer size */
    uint16_t        refcount;       /* Reference count */
    uint16_t        flags;          /* Buffer flags */

    /* Protocol info */
    uint16_t        protocol;       /* Protocol identifier */
    uint16_t        l2_offset;      /* L2 header offset */
    uint16_t        l3_offset;      /* L3 header offset */
    uint16_t        l4_offset;      /* L4 header offset */

    /* Networking */
    void            *netif;         /* Network interface */
    uint32_t        hash;           /* Flow hash */

    /* DMA info */
    addr_t          dma_addr;       /* Physical address for DMA */

    /* Linked list */
    struct zbuf     *next;
    struct zbuf     *prev;

    /* Timestamp for PROFINET RT */
    uint64_t        timestamp;

    /* Padding to align data */
    uint8_t         _pad[8];

    /* Inline data follows */
} zbuf_t;

/* Buffer Flags */
#define ZBUF_F_TX           (1 << 0)    /* TX buffer */
#define ZBUF_F_RX           (1 << 1)    /* RX buffer */
#define ZBUF_F_DMA          (1 << 2)    /* DMA capable */
#define ZBUF_F_SHARED       (1 << 3)    /* Shared buffer (don't free) */
#define ZBUF_F_CLONED       (1 << 4)    /* Cloned buffer */
#define ZBUF_F_CHECKSUM     (1 << 5)    /* Checksum offload */
#define ZBUF_F_TIMESTAMP    (1 << 6)    /* Has hardware timestamp */

/* Protocol IDs */
#define ZBUF_PROTO_ETH      0x0001
#define ZBUF_PROTO_IP       0x0800
#define ZBUF_PROTO_ARP      0x0806
#define ZBUF_PROTO_IP6      0x86DD
#define ZBUF_PROTO_VLAN     0x8100
#define ZBUF_PROTO_PROFINET 0x8892

/*
 * Buffer Pool
 */
typedef struct {
    zbuf_t          *free_list;
    spinlock_t      lock;
    uint32_t        total_count;
    uint32_t        free_count;
    uint32_t        alloc_failures;
    void            *pool_memory;
    size_t          buf_size;
} zbuf_pool_t;

/* Global buffer pool */
extern zbuf_pool_t zbuf_pool;

/*
 * API Functions
 */

/* Pool Management */
status_t zbuf_pool_init(void);
void zbuf_pool_stats(uint32_t *total, uint32_t *free, uint32_t *failures);

/* Buffer Allocation */
zbuf_t *zbuf_alloc(uint16_t size);
zbuf_t *zbuf_alloc_tx(uint16_t size);
zbuf_t *zbuf_alloc_rx(uint16_t size);
void zbuf_free(zbuf_t *zb);

/* Reference Counting */
zbuf_t *zbuf_ref(zbuf_t *zb);
void zbuf_unref(zbuf_t *zb);
zbuf_t *zbuf_clone(zbuf_t *zb);

/* Data Manipulation */
uint8_t *zbuf_push(zbuf_t *zb, uint16_t len);
uint8_t *zbuf_pull(zbuf_t *zb, uint16_t len);
uint8_t *zbuf_put(zbuf_t *zb, uint16_t len);
void zbuf_trim(zbuf_t *zb, uint16_t len);
void zbuf_reserve(zbuf_t *zb, uint16_t len);
void zbuf_reset(zbuf_t *zb);

/* Data Access */
static inline uint8_t *zbuf_data(zbuf_t *zb) { return zb->data; }
static inline uint16_t zbuf_len(zbuf_t *zb) { return zb->len; }
static inline uint16_t zbuf_headroom(zbuf_t *zb) { return zb->data - zb->head; }
static inline uint16_t zbuf_tailroom(zbuf_t *zb) { return zb->end - zb->tail; }

/* Protocol Headers */
static inline void *zbuf_l2_hdr(zbuf_t *zb) { return zb->data + zb->l2_offset; }
static inline void *zbuf_l3_hdr(zbuf_t *zb) { return zb->data + zb->l3_offset; }
static inline void *zbuf_l4_hdr(zbuf_t *zb) { return zb->data + zb->l4_offset; }

/* Buffer Queue */
typedef struct {
    zbuf_t          *head;
    zbuf_t          *tail;
    uint32_t        count;
    spinlock_t      lock;
} zbuf_queue_t;

void zbuf_queue_init(zbuf_queue_t *q);
void zbuf_queue_push(zbuf_queue_t *q, zbuf_t *zb);
zbuf_t *zbuf_queue_pop(zbuf_queue_t *q);
zbuf_t *zbuf_queue_peek(zbuf_queue_t *q);
uint32_t zbuf_queue_len(zbuf_queue_t *q);
void zbuf_queue_flush(zbuf_queue_t *q);

#endif /* ZBUF_H */
