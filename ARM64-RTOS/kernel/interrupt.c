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
 * ARM64 RTOS Kernel - Interrupt Management
 */

#include "rtos_types.h"
#include "rtos_config.h"

/* GIC Distributor Registers */
#define GICD_CTLR           (CONFIG_GICD_BASE + 0x000)
#define GICD_TYPER          (CONFIG_GICD_BASE + 0x004)
#define GICD_ISENABLER(n)   (CONFIG_GICD_BASE + 0x100 + (n) * 4)
#define GICD_ICENABLER(n)   (CONFIG_GICD_BASE + 0x180 + (n) * 4)
#define GICD_ISPENDR(n)     (CONFIG_GICD_BASE + 0x200 + (n) * 4)
#define GICD_ICPENDR(n)     (CONFIG_GICD_BASE + 0x280 + (n) * 4)
#define GICD_IPRIORITYR(n)  (CONFIG_GICD_BASE + 0x400 + (n))
#define GICD_ITARGETSR(n)   (CONFIG_GICD_BASE + 0x800 + (n))
#define GICD_ICFGR(n)       (CONFIG_GICD_BASE + 0xC00 + (n) * 4)

/* GIC CPU Interface Registers */
#define GICC_CTLR           (CONFIG_GICC_BASE + 0x000)
#define GICC_PMR            (CONFIG_GICC_BASE + 0x004)
#define GICC_BPR            (CONFIG_GICC_BASE + 0x008)
#define GICC_IAR            (CONFIG_GICC_BASE + 0x00C)
#define GICC_EOIR           (CONFIG_GICC_BASE + 0x010)
#define GICC_RPR            (CONFIG_GICC_BASE + 0x014)
#define GICC_HPPIR          (CONFIG_GICC_BASE + 0x018)

/* Memory-Mapped I/O */
#define mmio_read32(addr)       (*(volatile uint32_t *)(addr))
#define mmio_write32(addr, val) (*(volatile uint32_t *)(addr) = (val))

/* IRQ Handler Table */
#define MAX_IRQS    1024
static irq_entry_t irq_table[MAX_IRQS];
static spinlock_t irq_lock = SPINLOCK_INIT;

/* Nested Interrupt Counter */
static volatile uint32_t irq_nest_count = 0;

/*
 * Architecture IRQ Control
 */
uint64_t arch_irq_save(void)
{
    uint64_t flags;
    __asm__ volatile(
        "mrs %0, daif\n"
        "msr daifset, #2"
        : "=r"(flags) : : "memory"
    );
    return flags;
}

void arch_irq_restore(uint64_t flags)
{
    __asm__ volatile(
        "msr daif, %0"
        : : "r"(flags) : "memory"
    );
}

void arch_irq_enable(void)
{
    __asm__ volatile("msr daifclr, #2" ::: "memory");
}

void arch_irq_disable(void)
{
    __asm__ volatile("msr daifset, #2" ::: "memory");
}

bool arch_irq_enabled(void)
{
    uint64_t daif;
    __asm__ volatile("mrs %0, daif" : "=r"(daif));
    return (daif & 0x80) == 0;
}

/*
 * GIC Initialization
 */
void gic_init(void)
{
    uint32_t max_irqs;
    uint32_t typer;

    /* Disable distributor */
    mmio_write32(GICD_CTLR, 0);

    /* Get number of IRQs */
    typer = mmio_read32(GICD_TYPER);
    max_irqs = ((typer & 0x1F) + 1) * 32;
    if (max_irqs > MAX_IRQS) {
        max_irqs = MAX_IRQS;
    }

    /* Disable all IRQs */
    for (uint32_t i = 0; i < max_irqs / 32; i++) {
        mmio_write32(GICD_ICENABLER(i), 0xFFFFFFFF);
    }

    /* Clear pending */
    for (uint32_t i = 0; i < max_irqs / 32; i++) {
        mmio_write32(GICD_ICPENDR(i), 0xFFFFFFFF);
    }

    /* Set priority to lowest */
    for (uint32_t i = 0; i < max_irqs; i++) {
        mmio_write32(GICD_IPRIORITYR(i), 0xA0);
    }

    /* Target all SPIs to CPU 0 */
    for (uint32_t i = 32; i < max_irqs; i++) {
        mmio_write32(GICD_ITARGETSR(i), 0x01);
    }

    /* Configure all as level-triggered */
    for (uint32_t i = 2; i < max_irqs / 16; i++) {
        mmio_write32(GICD_ICFGR(i), 0);
    }

    /* Enable distributor */
    mmio_write32(GICD_CTLR, 1);

    /* Configure CPU interface */
    mmio_write32(GICC_PMR, 0xF0);   /* Priority mask */
    mmio_write32(GICC_BPR, 0);      /* Binary point */
    mmio_write32(GICC_CTLR, 1);     /* Enable */
}

/*
 * IRQ Enable/Disable
 */
void irq_enable(uint32_t irq)
{
    if (irq >= MAX_IRQS) return;
    mmio_write32(GICD_ISENABLER(irq / 32), 1 << (irq % 32));
}

void irq_disable(uint32_t irq)
{
    if (irq >= MAX_IRQS) return;
    mmio_write32(GICD_ICENABLER(irq / 32), 1 << (irq % 32));
}

void irq_set_priority(uint32_t irq, uint8_t priority)
{
    if (irq >= MAX_IRQS) return;
    mmio_write32(GICD_IPRIORITYR(irq), priority);
}

/*
 * IRQ Handler Registration
 */
status_t irq_register(uint32_t irq, irq_handler_t handler, void *arg)
{
    if (irq >= MAX_IRQS || handler == NULL) {
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
    irq_disable(irq);
    irq_table[irq].handler = NULL;
    irq_table[irq].arg = NULL;
    spin_unlock_irq(&irq_lock);

    return STATUS_OK;
}

/*
 * IRQ Handler (called from assembly vector)
 */
void irq_handler(void)
{
    uint32_t iar = mmio_read32(GICC_IAR);
    uint32_t irq = iar & 0x3FF;

    if (irq >= 1020) {
        /* Spurious interrupt */
        return;
    }

    irq_nest_count++;

    /* Call registered handler */
    if (irq < MAX_IRQS && irq_table[irq].handler != NULL) {
        irq_table[irq].handler(irq, irq_table[irq].arg);
    }

    /* End of interrupt */
    mmio_write32(GICC_EOIR, iar);

    irq_nest_count--;
}

/*
 * Check if in IRQ Context
 */
bool in_irq_context(void)
{
    return irq_nest_count > 0;
}

/*
 * Software Timer Management
 */
static timer_t *timer_list = NULL;
static spinlock_t timer_lock = SPINLOCK_INIT;

extern tick_t get_system_ticks(void);

void timer_init(timer_t *timer, timer_callback_t callback, void *arg)
{
    timer->callback = callback;
    timer->arg = arg;
    timer->active = false;
    timer->periodic = false;
    timer->next = NULL;
}

status_t timer_start(timer_t *timer, tick_t delay, bool periodic)
{
    if (timer == NULL || timer->callback == NULL) {
        return STATUS_INVALID;
    }

    spin_lock_irq(&timer_lock);

    /* Remove if already in list */
    timer_t **pp = &timer_list;
    while (*pp != NULL) {
        if (*pp == timer) {
            *pp = timer->next;
            break;
        }
        pp = &(*pp)->next;
    }

    /* Setup timer */
    timer->expire_tick = get_system_ticks() + delay;
    timer->period = periodic ? delay : 0;
    timer->periodic = periodic;
    timer->active = true;

    /* Insert sorted by expiration */
    pp = &timer_list;
    while (*pp != NULL && (*pp)->expire_tick <= timer->expire_tick) {
        pp = &(*pp)->next;
    }
    timer->next = *pp;
    *pp = timer;

    spin_unlock_irq(&timer_lock);
    return STATUS_OK;
}

void timer_stop(timer_t *timer)
{
    if (timer == NULL) return;

    spin_lock_irq(&timer_lock);

    timer->active = false;

    timer_t **pp = &timer_list;
    while (*pp != NULL) {
        if (*pp == timer) {
            *pp = timer->next;
            break;
        }
        pp = &(*pp)->next;
    }

    spin_unlock_irq(&timer_lock);
}

void timer_tick_handler(void)
{
    tick_t now = get_system_ticks();

    spin_lock(&timer_lock);

    while (timer_list != NULL && timer_list->expire_tick <= now) {
        timer_t *timer = timer_list;
        timer_list = timer->next;

        if (timer->active) {
            /* Call callback outside lock */
            spin_unlock(&timer_lock);
            timer->callback(timer->arg);
            spin_lock(&timer_lock);

            /* Reschedule if periodic */
            if (timer->periodic && timer->active) {
                timer->expire_tick = now + timer->period;

                /* Reinsert sorted */
                timer_t **pp = &timer_list;
                while (*pp != NULL && (*pp)->expire_tick <= timer->expire_tick) {
                    pp = &(*pp)->next;
                }
                timer->next = *pp;
                *pp = timer;
            }
        }
    }

    spin_unlock(&timer_lock);
}
