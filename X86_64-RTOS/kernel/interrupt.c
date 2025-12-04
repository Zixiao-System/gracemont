/*
 * Gracemont X86_64 RTOS - Interrupt Handler
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "rtos_types.h"
#include "x86_64/idt.h"
#include "x86_64/apic.h"

/* ============================================================================
 * External Functions
 * ============================================================================ */

extern void uart_puts(const char *s);
extern void uart_puthex(uint64_t val);

/* ============================================================================
 * IRQ Table
 * ============================================================================ */

#define MAX_IRQS 256

static irq_entry_t irq_table[MAX_IRQS];
static spinlock_t irq_lock = SPINLOCK_INIT;
static volatile uint32_t irq_nest_count = 0;

/* ============================================================================
 * IRQ State Management (implemented in assembly)
 * ============================================================================ */

extern bool arch_irq_enabled(void);

/* ============================================================================
 * IRQ Registration
 * ============================================================================ */

status_t irq_register(uint32_t irq, irq_handler_t handler, void *arg)
{
    if (irq >= MAX_IRQS) {
        return STATUS_INVALID;
    }

    spin_lock_irq(&irq_lock);

    irq_table[irq].handler = handler;
    irq_table[irq].arg = arg;

    spin_unlock_irq(&irq_lock);

    return STATUS_OK;
}

status_t irq_unregister(uint32_t irq)
{
    if (irq >= MAX_IRQS) {
        return STATUS_INVALID;
    }

    spin_lock_irq(&irq_lock);

    irq_table[irq].handler = NULL;
    irq_table[irq].arg = NULL;

    spin_unlock_irq(&irq_lock);

    return STATUS_OK;
}

/* ============================================================================
 * IRQ Handler (called from assembly via isr_common_stub)
 * ============================================================================ */

void irq_handler(interrupt_frame_t *frame)
{
    uint32_t int_no = frame->int_no;
    uint32_t irq = int_no - 32;  /* IRQs start at vector 32 */

    irq_nest_count++;

    /* Call registered handler if present */
    if (int_no < MAX_IRQS && irq_table[int_no].handler != NULL) {
        irq_table[int_no].handler(irq, irq_table[int_no].arg);
    }

    /* Send EOI to APIC */
    apic_send_eoi();

    irq_nest_count--;
}

/* ============================================================================
 * IRQ Enable/Disable
 * ============================================================================ */

void irq_enable(uint32_t irq)
{
    apic_enable_irq(irq);
}

void irq_disable(uint32_t irq)
{
    apic_disable_irq(irq);
}

/* ============================================================================
 * System Tick Counter
 * ============================================================================ */

static volatile tick_t system_ticks = 0;

tick_t get_system_ticks(void)
{
    return system_ticks;
}

/* Timer tick handler - called from APIC timer IRQ */
static void timer_tick_handler(uint32_t irq, void *arg)
{
    (void)irq;
    (void)arg;

    system_ticks++;

    /* TODO: Call scheduler tick */
    /* scheduler_tick(); */
}

/* ============================================================================
 * Initialize Interrupt System
 * ============================================================================ */

void interrupt_init(void)
{
    /* Clear IRQ table */
    for (int i = 0; i < MAX_IRQS; i++) {
        irq_table[i].handler = NULL;
        irq_table[i].arg = NULL;
    }

    /* Register timer tick handler (vector 32 = APIC timer) */
    irq_register(32, timer_tick_handler, NULL);
}
