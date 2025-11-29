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
 * UART Driver Header
 */

#ifndef UART_H
#define UART_H

#include "rtos_types.h"

/*
 * UART Parity Options
 */
typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_ODD,
    UART_PARITY_EVEN
} uart_parity_t;

/*
 * UART Configuration
 */
typedef struct {
    uint8_t         data_bits;      /* 5, 6, 7, 8 */
    uint8_t         stop_bits;      /* 1, 2 */
    uart_parity_t   parity;
    bool            hw_flow_ctrl;   /* Hardware flow control (RTS/CTS) */
} uart_config_t;

/*
 * UART Statistics
 */
typedef struct {
    uint32_t    rx_count;
    uint32_t    tx_count;
    uint32_t    rx_errors;
    uint32_t    tx_errors;
} uart_stats_t;

/*
 * RX Callback
 */
typedef void (*uart_rx_callback_t)(void *arg, uint8_t data);

/*
 * API Functions
 */

/* Initialize UART port */
status_t uart_init(uint32_t port, uint32_t baud, uart_config_t *config);

/* Deinitialize UART port */
void uart_deinit(uint32_t port);

/* Blocking write */
int uart_write(uint32_t port, const uint8_t *data, size_t len);

/* Blocking read */
int uart_read(uint32_t port, uint8_t *data, size_t len);

/* Polling write single character */
void uart_putc(uint32_t port, char c);

/* Polling read single character (-1 if none) */
int uart_getc(uint32_t port);

/* Polling write string */
void uart_puts(uint32_t port, const char *s);

/* Check if data available */
bool uart_readable(uint32_t port);

/* Check if TX has space */
bool uart_writable(uint32_t port);

/* Flush TX buffer */
void uart_flush(uint32_t port);

/* Set RX callback */
void uart_set_rx_callback(uint32_t port, uart_rx_callback_t callback, void *arg);

/* Get statistics */
void uart_get_stats(uint32_t port, uart_stats_t *stats);

/*
 * Default UART Configuration
 */
#define UART_CONFIG_DEFAULT { \
    .data_bits = 8,           \
    .stop_bits = 1,           \
    .parity = UART_PARITY_NONE, \
    .hw_flow_ctrl = false     \
}

#endif /* UART_H */
