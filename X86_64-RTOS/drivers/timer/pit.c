/*
 * Gracemont X86_64 RTOS - PIT Driver (Programmable Interval Timer)
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "x86_64/cpu.h"
#include "rtos_types.h"

/* PIT I/O Ports */
#define PIT_CH0_DATA    0x40
#define PIT_CH1_DATA    0x41
#define PIT_CH2_DATA    0x42
#define PIT_CMD         0x43

/* PIT Frequency */
#define PIT_FREQUENCY   1193182

/* PIT Command Bits */
#define PIT_CMD_CH0     0x00
#define PIT_CMD_CH1     0x40
#define PIT_CMD_CH2     0x80
#define PIT_CMD_LATCH   0x00
#define PIT_CMD_LOBYTE  0x10
#define PIT_CMD_HIBYTE  0x20
#define PIT_CMD_BOTH    0x30
#define PIT_CMD_MODE0   0x00    /* Interrupt on terminal count */
#define PIT_CMD_MODE2   0x04    /* Rate generator */
#define PIT_CMD_MODE3   0x06    /* Square wave */

/*
 * Initialize PIT
 */
void pit_init(uint32_t frequency)
{
    uint16_t divisor = PIT_FREQUENCY / frequency;

    /* Channel 0, mode 2 (rate generator), both bytes */
    outb(PIT_CMD, PIT_CMD_CH0 | PIT_CMD_BOTH | PIT_CMD_MODE2);

    /* Set divisor */
    outb(PIT_CH0_DATA, divisor & 0xFF);         /* Low byte */
    outb(PIT_CH0_DATA, (divisor >> 8) & 0xFF);  /* High byte */
}

/*
 * Read PIT count
 */
uint16_t pit_read_count(void)
{
    uint16_t count;

    /* Latch channel 0 */
    outb(PIT_CMD, PIT_CMD_LATCH);

    /* Read count */
    count = inb(PIT_CH0_DATA);
    count |= ((uint16_t)inb(PIT_CH0_DATA)) << 8;

    return count;
}

/*
 * Simple delay using PIT (for calibration)
 */
void pit_delay_ms(uint32_t ms)
{
    /* Each tick is ~838ns (1/1193182 seconds) */
    uint32_t ticks = (PIT_FREQUENCY / 1000) * ms;

    /* Set up one-shot mode */
    outb(PIT_CMD, PIT_CMD_CH0 | PIT_CMD_BOTH | PIT_CMD_MODE0);

    if (ticks > 65535) ticks = 65535;

    outb(PIT_CH0_DATA, ticks & 0xFF);
    outb(PIT_CH0_DATA, (ticks >> 8) & 0xFF);

    /* Wait for count to reach 0 */
    while (pit_read_count() > 0) {
        __asm__ volatile("pause");
    }
}
