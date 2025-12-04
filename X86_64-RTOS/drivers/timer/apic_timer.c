/*
 * Gracemont X86_64 RTOS - APIC Timer Driver
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This is a thin wrapper around the APIC timer functionality in apic.c
 */

#include "x86_64/apic.h"
#include "rtos_types.h"

/* Timer state */
static volatile tick_t timer_ticks = 0;
static uint32_t timer_frequency = 0;

/*
 * Timer IRQ Handler
 */
static void apic_timer_irq_handler(uint32_t irq, void *arg)
{
    (void)irq;
    (void)arg;

    timer_ticks++;

    /* Call scheduler tick */
    extern void scheduler_tick(void);
    scheduler_tick();
}

/*
 * Initialize APIC Timer
 */
status_t apic_timer_driver_init(uint32_t freq)
{
    timer_frequency = freq;
    timer_ticks = 0;

    /* Register IRQ handler */
    extern status_t irq_register(uint32_t irq, irq_handler_t handler, void *arg);
    irq_register(32, apic_timer_irq_handler, NULL);  /* APIC timer is vector 32 */

    /* Initialize and start the timer */
    apic_timer_init(freq);

    return STATUS_OK;
}

/*
 * Get timer ticks
 */
tick_t apic_timer_get_ticks(void)
{
    return timer_ticks;
}

/*
 * Timer delay (busy wait)
 */
void apic_timer_delay_ms(uint32_t ms)
{
    tick_t target = timer_ticks + (ms * timer_frequency / 1000);
    while (timer_ticks < target) {
        __asm__ volatile("pause");
    }
}
