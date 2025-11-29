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
 * PL011 UART Driver for ARM64
 */

#include "uart.h"
#include "rtos_config.h"
#include "rtos.h"

/*
 * PL011 UART Register Offsets
 */
#define UART_DR         0x00    /* Data Register */
#define UART_RSR        0x04    /* Receive Status Register */
#define UART_FR         0x18    /* Flag Register */
#define UART_ILPR       0x20    /* IrDA Low-Power Counter */
#define UART_IBRD       0x24    /* Integer Baud Rate */
#define UART_FBRD       0x28    /* Fractional Baud Rate */
#define UART_LCR_H      0x2C    /* Line Control */
#define UART_CR         0x30    /* Control Register */
#define UART_IFLS       0x34    /* Interrupt FIFO Level */
#define UART_IMSC       0x38    /* Interrupt Mask */
#define UART_RIS        0x3C    /* Raw Interrupt Status */
#define UART_MIS        0x40    /* Masked Interrupt Status */
#define UART_ICR        0x44    /* Interrupt Clear */
#define UART_DMACR      0x48    /* DMA Control */

/*
 * Flag Register bits
 */
#define FR_TXFE         (1 << 7)    /* TX FIFO Empty */
#define FR_RXFF         (1 << 6)    /* RX FIFO Full */
#define FR_TXFF         (1 << 5)    /* TX FIFO Full */
#define FR_RXFE         (1 << 4)    /* RX FIFO Empty */
#define FR_BUSY         (1 << 3)    /* UART Busy */

/*
 * Control Register bits
 */
#define CR_CTSEN        (1 << 15)   /* CTS Enable */
#define CR_RTSEN        (1 << 14)   /* RTS Enable */
#define CR_RTS          (1 << 11)   /* RTS */
#define CR_RXE          (1 << 9)    /* RX Enable */
#define CR_TXE          (1 << 8)    /* TX Enable */
#define CR_LBE          (1 << 7)    /* Loopback Enable */
#define CR_UARTEN       (1 << 0)    /* UART Enable */

/*
 * Line Control bits
 */
#define LCR_H_SPS       (1 << 7)    /* Stick Parity */
#define LCR_H_WLEN_8    (3 << 5)    /* 8-bit word */
#define LCR_H_WLEN_7    (2 << 5)    /* 7-bit word */
#define LCR_H_WLEN_6    (1 << 5)    /* 6-bit word */
#define LCR_H_WLEN_5    (0 << 5)    /* 5-bit word */
#define LCR_H_FEN       (1 << 4)    /* FIFO Enable */
#define LCR_H_STP2      (1 << 3)    /* Two Stop Bits */
#define LCR_H_EPS       (1 << 2)    /* Even Parity */
#define LCR_H_PEN       (1 << 1)    /* Parity Enable */
#define LCR_H_BRK       (1 << 0)    /* Send Break */

/*
 * Interrupt bits
 */
#define INT_OE          (1 << 10)   /* Overrun Error */
#define INT_BE          (1 << 9)    /* Break Error */
#define INT_PE          (1 << 8)    /* Parity Error */
#define INT_FE          (1 << 7)    /* Framing Error */
#define INT_RT          (1 << 6)    /* Receive Timeout */
#define INT_TX          (1 << 5)    /* TX Interrupt */
#define INT_RX          (1 << 4)    /* RX Interrupt */

/*
 * Register access macros
 */
#define UART_REG(uart, offset)  (*(volatile uint32_t *)((uart)->base + (offset)))

/*
 * RX/TX Ring Buffer Size
 */
#define UART_BUFFER_SIZE    256

/*
 * UART Device Context
 */
typedef struct {
    addr_t          base;
    uint32_t        irq;
    uint32_t        clock;
    uint32_t        baud;

    /* RX Ring Buffer */
    uint8_t         rx_buffer[UART_BUFFER_SIZE];
    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;

    /* TX Ring Buffer */
    uint8_t         tx_buffer[UART_BUFFER_SIZE];
    volatile uint16_t tx_head;
    volatile uint16_t tx_tail;
    volatile bool   tx_active;

    /* Synchronization */
    semaphore_t     rx_sem;
    semaphore_t     tx_sem;
    spinlock_t      lock;

    /* Callbacks */
    uart_rx_callback_t rx_callback;
    void            *rx_callback_arg;

    /* Statistics */
    uint32_t        rx_count;
    uint32_t        tx_count;
    uint32_t        rx_errors;
    uint32_t        tx_errors;

    bool            initialized;
} uart_dev_t;

/* UART device instances */
static uart_dev_t uart_devices[CONFIG_UART_COUNT];

/*
 * IRQ Handler
 */
static void uart_irq_handler(uint32_t irq, void *arg)
{
    uart_dev_t *dev = (uart_dev_t *)arg;
    uint32_t mis = UART_REG(dev, UART_MIS);

    /* RX Interrupt */
    if (mis & (INT_RX | INT_RT)) {
        while (!(UART_REG(dev, UART_FR) & FR_RXFE)) {
            uint32_t data = UART_REG(dev, UART_DR);

            /* Check for errors */
            if (data & 0xF00) {
                dev->rx_errors++;
                /* Clear error flags */
                UART_REG(dev, UART_RSR) = 0;
                continue;
            }

            /* Store in buffer */
            uint16_t next_head = (dev->rx_head + 1) % UART_BUFFER_SIZE;
            if (next_head != dev->rx_tail) {
                dev->rx_buffer[dev->rx_head] = (uint8_t)data;
                dev->rx_head = next_head;
                dev->rx_count++;

                /* Signal waiting task */
                sem_post(&dev->rx_sem);

                /* Callback */
                if (dev->rx_callback) {
                    dev->rx_callback(dev->rx_callback_arg, (uint8_t)data);
                }
            }
        }

        /* Clear RX interrupt */
        UART_REG(dev, UART_ICR) = INT_RX | INT_RT;
    }

    /* TX Interrupt */
    if (mis & INT_TX) {
        spin_lock(&dev->lock);

        /* Send more data if available */
        while (!(UART_REG(dev, UART_FR) & FR_TXFF) &&
               dev->tx_tail != dev->tx_head) {
            UART_REG(dev, UART_DR) = dev->tx_buffer[dev->tx_tail];
            dev->tx_tail = (dev->tx_tail + 1) % UART_BUFFER_SIZE;
            dev->tx_count++;

            /* Signal waiting task */
            sem_post(&dev->tx_sem);
        }

        /* Disable TX interrupt if buffer empty */
        if (dev->tx_tail == dev->tx_head) {
            uint32_t imsc = UART_REG(dev, UART_IMSC);
            UART_REG(dev, UART_IMSC) = imsc & ~INT_TX;
            dev->tx_active = false;
        }

        spin_unlock(&dev->lock);

        /* Clear TX interrupt */
        UART_REG(dev, UART_ICR) = INT_TX;
    }

    /* Error Interrupts */
    if (mis & (INT_OE | INT_BE | INT_PE | INT_FE)) {
        dev->rx_errors++;
        UART_REG(dev, UART_ICR) = INT_OE | INT_BE | INT_PE | INT_FE;
    }
}

/*
 * Calculate Baud Rate Divisors
 */
static void uart_set_baudrate(uart_dev_t *dev, uint32_t baud)
{
    /*
     * Baud rate divisor = UARTCLK / (16 * Baud Rate)
     * IBRD = integer part
     * FBRD = fractional part * 64 + 0.5
     */
    uint32_t divider = dev->clock / (16 * baud);
    uint32_t remainder = dev->clock % (16 * baud);
    uint32_t fraction = ((8 * remainder) / baud + 1) / 2;

    UART_REG(dev, UART_IBRD) = divider;
    UART_REG(dev, UART_FBRD) = fraction;
}

/*
 * Initialize UART
 */
status_t uart_init(uint32_t port, uint32_t baud, uart_config_t *config)
{
    if (port >= CONFIG_UART_COUNT) {
        return STATUS_INVALID;
    }

    uart_dev_t *dev = &uart_devices[port];

    /* Set base address based on port */
    switch (port) {
    case 0:
        dev->base = CONFIG_UART_BASE;
        dev->irq = CONFIG_UART_IRQ;
        break;
    default:
        return STATUS_INVALID;
    }

    dev->clock = CONFIG_UART_CLOCK;
    dev->baud = baud;

    /* Initialize buffers */
    dev->rx_head = 0;
    dev->rx_tail = 0;
    dev->tx_head = 0;
    dev->tx_tail = 0;
    dev->tx_active = false;

    /* Initialize synchronization */
    sem_init(&dev->rx_sem, 0);
    sem_init(&dev->tx_sem, UART_BUFFER_SIZE);
    dev->lock = (spinlock_t)SPINLOCK_INIT;

    /* Reset counters */
    dev->rx_count = 0;
    dev->tx_count = 0;
    dev->rx_errors = 0;
    dev->tx_errors = 0;
    dev->rx_callback = NULL;

    /* Disable UART */
    UART_REG(dev, UART_CR) = 0;

    /* Wait for TX to complete */
    while (UART_REG(dev, UART_FR) & FR_BUSY);

    /* Disable interrupts */
    UART_REG(dev, UART_IMSC) = 0;

    /* Clear pending interrupts */
    UART_REG(dev, UART_ICR) = 0x7FF;

    /* Set baud rate */
    uart_set_baudrate(dev, baud);

    /* Configure line control */
    uint32_t lcr_h = LCR_H_FEN;  /* Enable FIFO */

    if (config) {
        /* Word length */
        switch (config->data_bits) {
        case 5: lcr_h |= LCR_H_WLEN_5; break;
        case 6: lcr_h |= LCR_H_WLEN_6; break;
        case 7: lcr_h |= LCR_H_WLEN_7; break;
        case 8:
        default: lcr_h |= LCR_H_WLEN_8; break;
        }

        /* Parity */
        if (config->parity != UART_PARITY_NONE) {
            lcr_h |= LCR_H_PEN;
            if (config->parity == UART_PARITY_EVEN) {
                lcr_h |= LCR_H_EPS;
            }
        }

        /* Stop bits */
        if (config->stop_bits == 2) {
            lcr_h |= LCR_H_STP2;
        }
    } else {
        /* Default: 8N1 */
        lcr_h |= LCR_H_WLEN_8;
    }

    UART_REG(dev, UART_LCR_H) = lcr_h;

    /* Set FIFO interrupt levels */
    UART_REG(dev, UART_IFLS) = 0;  /* RX 1/8, TX 1/8 */

    /* Enable RX interrupt */
    UART_REG(dev, UART_IMSC) = INT_RX | INT_RT | INT_OE | INT_BE | INT_PE | INT_FE;

    /* Register IRQ handler */
    irq_register(dev->irq, uart_irq_handler, dev);
    irq_enable(dev->irq);

    /* Enable UART */
    UART_REG(dev, UART_CR) = CR_UARTEN | CR_TXE | CR_RXE;

    dev->initialized = true;

    return STATUS_OK;
}

/*
 * Deinitialize UART
 */
void uart_deinit(uint32_t port)
{
    if (port >= CONFIG_UART_COUNT) return;

    uart_dev_t *dev = &uart_devices[port];
    if (!dev->initialized) return;

    /* Disable UART */
    UART_REG(dev, UART_CR) = 0;

    /* Disable interrupts */
    UART_REG(dev, UART_IMSC) = 0;
    irq_disable(dev->irq);

    dev->initialized = false;
}

/*
 * Transmit data (blocking)
 */
int uart_write(uint32_t port, const uint8_t *data, size_t len)
{
    if (port >= CONFIG_UART_COUNT || data == NULL) {
        return -1;
    }

    uart_dev_t *dev = &uart_devices[port];
    if (!dev->initialized) return -1;

    size_t sent = 0;

    while (sent < len) {
        /* Wait for space in buffer */
        sem_wait(&dev->tx_sem);

        spin_lock_irq(&dev->lock);

        /* Add to buffer */
        uint16_t next_head = (dev->tx_head + 1) % UART_BUFFER_SIZE;
        if (next_head != dev->tx_tail) {
            dev->tx_buffer[dev->tx_head] = data[sent++];
            dev->tx_head = next_head;
        }

        /* Start transmission if not active */
        if (!dev->tx_active) {
            dev->tx_active = true;

            /* Fill TX FIFO */
            while (!(UART_REG(dev, UART_FR) & FR_TXFF) &&
                   dev->tx_tail != dev->tx_head) {
                UART_REG(dev, UART_DR) = dev->tx_buffer[dev->tx_tail];
                dev->tx_tail = (dev->tx_tail + 1) % UART_BUFFER_SIZE;
                dev->tx_count++;
                sem_post(&dev->tx_sem);
            }

            /* Enable TX interrupt */
            uint32_t imsc = UART_REG(dev, UART_IMSC);
            UART_REG(dev, UART_IMSC) = imsc | INT_TX;
        }

        spin_unlock_irq(&dev->lock);
    }

    return (int)sent;
}

/*
 * Receive data (blocking)
 */
int uart_read(uint32_t port, uint8_t *data, size_t len)
{
    if (port >= CONFIG_UART_COUNT || data == NULL) {
        return -1;
    }

    uart_dev_t *dev = &uart_devices[port];
    if (!dev->initialized) return -1;

    size_t received = 0;

    while (received < len) {
        /* Wait for data */
        sem_wait(&dev->rx_sem);

        spin_lock_irq(&dev->lock);

        if (dev->rx_tail != dev->rx_head) {
            data[received++] = dev->rx_buffer[dev->rx_tail];
            dev->rx_tail = (dev->rx_tail + 1) % UART_BUFFER_SIZE;
        }

        spin_unlock_irq(&dev->lock);
    }

    return (int)received;
}

/*
 * Write single character (polling)
 */
void uart_putc(uint32_t port, char c)
{
    if (port >= CONFIG_UART_COUNT) return;

    uart_dev_t *dev = &uart_devices[port];

    /* Wait for TX FIFO not full */
    while (UART_REG(dev, UART_FR) & FR_TXFF);

    UART_REG(dev, UART_DR) = c;
}

/*
 * Read single character (polling)
 */
int uart_getc(uint32_t port)
{
    if (port >= CONFIG_UART_COUNT) return -1;

    uart_dev_t *dev = &uart_devices[port];

    /* Check for data */
    if (UART_REG(dev, UART_FR) & FR_RXFE) {
        return -1;
    }

    return UART_REG(dev, UART_DR) & 0xFF;
}

/*
 * Write string (polling)
 */
void uart_puts(uint32_t port, const char *s)
{
    if (port >= CONFIG_UART_COUNT || s == NULL) return;

    while (*s) {
        if (*s == '\n') {
            uart_putc(port, '\r');
        }
        uart_putc(port, *s++);
    }
}

/*
 * Check if data available
 */
bool uart_readable(uint32_t port)
{
    if (port >= CONFIG_UART_COUNT) return false;

    uart_dev_t *dev = &uart_devices[port];
    return dev->rx_head != dev->rx_tail;
}

/*
 * Check if TX buffer has space
 */
bool uart_writable(uint32_t port)
{
    if (port >= CONFIG_UART_COUNT) return false;

    uart_dev_t *dev = &uart_devices[port];
    return ((dev->tx_head + 1) % UART_BUFFER_SIZE) != dev->tx_tail;
}

/*
 * Flush TX buffer
 */
void uart_flush(uint32_t port)
{
    if (port >= CONFIG_UART_COUNT) return;

    uart_dev_t *dev = &uart_devices[port];

    /* Wait for TX buffer empty */
    while (dev->tx_tail != dev->tx_head) {
        task_yield();
    }

    /* Wait for TX FIFO empty */
    while (!(UART_REG(dev, UART_FR) & FR_TXFE));

    /* Wait for UART not busy */
    while (UART_REG(dev, UART_FR) & FR_BUSY);
}

/*
 * Set RX callback
 */
void uart_set_rx_callback(uint32_t port, uart_rx_callback_t callback, void *arg)
{
    if (port >= CONFIG_UART_COUNT) return;

    uart_dev_t *dev = &uart_devices[port];
    dev->rx_callback = callback;
    dev->rx_callback_arg = arg;
}

/*
 * Get statistics
 */
void uart_get_stats(uint32_t port, uart_stats_t *stats)
{
    if (port >= CONFIG_UART_COUNT || stats == NULL) return;

    uart_dev_t *dev = &uart_devices[port];
    stats->rx_count = dev->rx_count;
    stats->tx_count = dev->tx_count;
    stats->rx_errors = dev->rx_errors;
    stats->tx_errors = dev->tx_errors;
}
