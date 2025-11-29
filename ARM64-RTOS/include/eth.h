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
 * Ethernet Driver Header
 */

#ifndef ETH_H
#define ETH_H

#include "rtos_types.h"
#include "net_stack.h"

/*
 * Ethernet Statistics
 */
typedef struct {
    uint64_t    rx_packets;
    uint64_t    tx_packets;
    uint64_t    rx_bytes;
    uint64_t    tx_bytes;
    uint64_t    rx_errors;
    uint64_t    tx_errors;
    uint64_t    rx_dropped;
} eth_stats_t;

/*
 * API Functions
 */

/* Initialize Ethernet driver */
status_t eth_init(void);

/* Get network interface */
netif_t *eth_get_netif(void);

/* Get statistics */
void eth_get_stats(eth_stats_t *stats);

/* Poll for packets (non-interrupt mode) */
void eth_poll(void);

#endif /* ETH_H */
