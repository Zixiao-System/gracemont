/*
 * Gracemont X86_64 RTOS - 16550 UART Driver
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "x86_64/cpu.h"
#include "rtos_types.h"

/* ============================================================================
 * UART Registers (relative to base port)
 * ============================================================================ */

#define UART_RBR    0   /* Receive Buffer Register (read) */
#define UART_THR    0   /* Transmit Holding Register (write) */
#define UART_DLL    0   /* Divisor Latch Low (DLAB=1) */
#define UART_IER    1   /* Interrupt Enable Register */
#define UART_DLH    1   /* Divisor Latch High (DLAB=1) */
#define UART_IIR    2   /* Interrupt Identification (read) */
#define UART_FCR    2   /* FIFO Control Register (write) */
#define UART_LCR    3   /* Line Control Register */
#define UART_MCR    4   /* Modem Control Register */
#define UART_LSR    5   /* Line Status Register */
#define UART_MSR    6   /* Modem Status Register */
#define UART_SCR    7   /* Scratch Register */

/* Line Status Register bits */
#define UART_LSR_DR     0x01    /* Data Ready */
#define UART_LSR_OE     0x02    /* Overrun Error */
#define UART_LSR_PE     0x04    /* Parity Error */
#define UART_LSR_FE     0x08    /* Framing Error */
#define UART_LSR_BI     0x10    /* Break Interrupt */
#define UART_LSR_THRE   0x20    /* Transmit Holding Register Empty */
#define UART_LSR_TEMT   0x40    /* Transmitter Empty */

/* Line Control Register bits */
#define UART_LCR_DLAB   0x80    /* Divisor Latch Access Bit */

/* FIFO Control Register bits */
#define UART_FCR_ENABLE     0x01    /* Enable FIFOs */
#define UART_FCR_CLEAR_RX   0x02    /* Clear Receive FIFO */
#define UART_FCR_CLEAR_TX   0x04    /* Clear Transmit FIFO */
#define UART_FCR_TRIGGER_14 0xC0    /* Trigger at 14 bytes */

/* Modem Control Register bits */
#define UART_MCR_DTR    0x01    /* Data Terminal Ready */
#define UART_MCR_RTS    0x02    /* Request To Send */
#define UART_MCR_OUT1   0x04    /* Out 1 */
#define UART_MCR_OUT2   0x08    /* Out 2 (enables IRQ) */

/* ============================================================================
 * COM Port Addresses
 * ============================================================================ */

#define COM1_PORT   0x3F8
#define COM2_PORT   0x2F8
#define COM3_PORT   0x3E8
#define COM4_PORT   0x2E8

/* ============================================================================
 * Current UART Port
 * ============================================================================ */

static uint16_t uart_port = COM1_PORT;

/* ============================================================================
 * UART Functions
 * ============================================================================ */

void uart_init_port(uint16_t port, uint32_t baud)
{
    uint16_t divisor = 115200 / baud;

    uart_port = port;

    /* Disable interrupts */
    outb(port + UART_IER, 0x00);

    /* Enable DLAB (Divisor Latch Access) */
    outb(port + UART_LCR, UART_LCR_DLAB);

    /* Set baud rate divisor */
    outb(port + UART_DLL, divisor & 0xFF);
    outb(port + UART_DLH, (divisor >> 8) & 0xFF);

    /* 8 bits, no parity, 1 stop bit (8N1) */
    outb(port + UART_LCR, 0x03);

    /* Enable and clear FIFOs, 14-byte trigger */
    outb(port + UART_FCR, UART_FCR_ENABLE | UART_FCR_CLEAR_RX |
                          UART_FCR_CLEAR_TX | UART_FCR_TRIGGER_14);

    /* Enable IRQ, RTS/DSR set */
    outb(port + UART_MCR, UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2);

    /* Test the chip (should return the same byte) */
    outb(port + UART_SCR, 0xAE);
    if (inb(port + UART_SCR) != 0xAE) {
        /* UART not present or faulty */
        return;
    }

    /* Clear any pending interrupts */
    (void)inb(port + UART_IIR);
    (void)inb(port + UART_RBR);
}

void uart_init(void)
{
    uart_init_port(COM1_PORT, 115200);
}

static int uart_is_transmit_empty(void)
{
    return inb(uart_port + UART_LSR) & UART_LSR_THRE;
}

static int uart_is_received(void)
{
    return inb(uart_port + UART_LSR) & UART_LSR_DR;
}

void uart_putc(char c)
{
    /* Wait for transmit buffer to be empty */
    while (!uart_is_transmit_empty()) {
        __asm__ volatile("pause");
    }
    outb(uart_port + UART_THR, c);
}

char uart_getc(void)
{
    /* Wait for data to be received */
    while (!uart_is_received()) {
        __asm__ volatile("pause");
    }
    return inb(uart_port + UART_RBR);
}

int uart_getc_nonblock(void)
{
    if (uart_is_received()) {
        return inb(uart_port + UART_RBR);
    }
    return -1;
}

void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

/* ============================================================================
 * Hex Output for Debugging
 * ============================================================================ */

static const char hex_chars[] = "0123456789ABCDEF";

void uart_puthex(uint64_t val)
{
    uart_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uart_putc(hex_chars[(val >> i) & 0xF]);
    }
}

void uart_puthex32(uint32_t val)
{
    uart_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uart_putc(hex_chars[(val >> i) & 0xF]);
    }
}

void uart_putdec(uint64_t val)
{
    char buf[21];
    int i = 20;
    buf[i] = '\0';

    if (val == 0) {
        uart_putc('0');
        return;
    }

    while (val > 0 && i > 0) {
        buf[--i] = '0' + (val % 10);
        val /= 10;
    }

    uart_puts(&buf[i]);
}
