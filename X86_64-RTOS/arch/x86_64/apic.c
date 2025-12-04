/*
 * Gracemont X86_64 RTOS - APIC Implementation
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "x86_64/apic.h"
#include "x86_64/cpu.h"
#include "x86_64/paging.h"
#include "rtos_types.h"

/* ============================================================================
 * APIC Base Addresses
 * ============================================================================ */

static volatile uint32_t *lapic_base = NULL;
static volatile uint32_t *ioapic_base = NULL;

/* ============================================================================
 * Local APIC Access
 * ============================================================================ */

uint32_t lapic_read(uint32_t reg)
{
    return lapic_base[reg / 4];
}

void lapic_write(uint32_t reg, uint32_t val)
{
    lapic_base[reg / 4] = val;
    /* Read back to ensure write completes */
    (void)lapic_base[LAPIC_ID / 4];
}

uint32_t lapic_get_id(void)
{
    return (lapic_read(LAPIC_ID) >> 24) & 0xFF;
}

void lapic_eoi(void)
{
    lapic_write(LAPIC_EOI, 0);
}

/* ============================================================================
 * I/O APIC Access
 * ============================================================================ */

uint32_t ioapic_read(uint32_t reg)
{
    ioapic_base[IOAPIC_REGSEL / 4] = reg;
    return ioapic_base[IOAPIC_REGWIN / 4];
}

void ioapic_write(uint32_t reg, uint32_t val)
{
    ioapic_base[IOAPIC_REGSEL / 4] = reg;
    ioapic_base[IOAPIC_REGWIN / 4] = val;
}

/* ============================================================================
 * APIC Initialization
 * ============================================================================ */

void apic_init(void)
{
    /* Get APIC base address from MSR */
    uint64_t apic_msr = rdmsr(MSR_IA32_APIC_BASE);

    /* Check if APIC is enabled */
    if (!(apic_msr & (1 << 11))) {
        /* Enable APIC */
        apic_msr |= (1 << 11);
        wrmsr(MSR_IA32_APIC_BASE, apic_msr);
    }

    /* Get base address (bits 12-35, page aligned) */
    uint64_t base_phys = apic_msr & 0xFFFFFF000UL;
    lapic_base = (volatile uint32_t*)base_phys;

    /* Set up Spurious Interrupt Vector Register */
    /* Enable APIC and set spurious vector to 255 */
    lapic_write(LAPIC_SVR, LAPIC_SVR_ENABLE | APIC_SPURIOUS_VECTOR);

    /* Clear Error Status Register (write twice) */
    lapic_write(LAPIC_ESR, 0);
    lapic_write(LAPIC_ESR, 0);

    /* Set Task Priority to 0 (accept all interrupts) */
    lapic_write(LAPIC_TPR, 0);

    /* Mask all LVT entries initially */
    lapic_write(LAPIC_LVT_TIMER, LAPIC_LVT_MASKED);
    lapic_write(LAPIC_LVT_THERMAL, LAPIC_LVT_MASKED);
    lapic_write(LAPIC_LVT_PERF, LAPIC_LVT_MASKED);
    lapic_write(LAPIC_LVT_LINT0, LAPIC_LVT_MASKED);
    lapic_write(LAPIC_LVT_LINT1, LAPIC_LVT_MASKED);
    lapic_write(LAPIC_LVT_ERROR, APIC_ERROR_VECTOR);

    /* Send EOI to clear any pending interrupts */
    lapic_eoi();
}

void ioapic_init(void)
{
    /* Use default I/O APIC base address */
    /* In a real system, this would come from ACPI MADT table */
    ioapic_base = (volatile uint32_t*)IOAPIC_DEFAULT_BASE;

    /* Read I/O APIC version to verify it's present */
    uint32_t ver = ioapic_read(IOAPIC_VER);
    uint32_t max_irq = ((ver >> 16) & 0xFF) + 1;

    /* Mask all IRQs initially */
    for (uint32_t i = 0; i < max_irq; i++) {
        ioapic_set_irq(i, 32 + i, true);  /* Masked, vector = 32 + IRQ */
    }
}

/* ============================================================================
 * I/O APIC IRQ Configuration
 * ============================================================================ */

void ioapic_set_irq(uint8_t irq, uint8_t vector, bool masked)
{
    uint32_t reg = IOAPIC_REDTBL(irq);

    /* Low 32 bits: vector, delivery mode, destination mode, etc. */
    uint32_t low = vector;
    if (masked) {
        low |= IOAPIC_MASKED;
    }

    /* High 32 bits: destination (CPU 0) */
    uint32_t high = 0;  /* CPU 0 in physical mode */

    ioapic_write(reg, low);
    ioapic_write(reg + 1, high);
}

/* ============================================================================
 * APIC Timer
 * ============================================================================ */

static uint32_t apic_timer_ticks_per_ms = 0;

void apic_timer_init(uint32_t frequency)
{
    /* Calibrate APIC timer using PIT */

    /* Set up PIT channel 2 for calibration */
    /* We'll use a simple busy-wait calibration */

    /* Set timer divide to 16 */
    lapic_write(LAPIC_TIMER_DCR, LAPIC_TIMER_DIV_16);

    /* Set initial count to max */
    lapic_write(LAPIC_TIMER_ICR, 0xFFFFFFFF);

    /* Wait approximately 10ms using a simple delay loop */
    /* This is a rough estimate - in a real system we'd use PIT or HPET */
    for (volatile int i = 0; i < 10000000; i++) {
        __asm__ volatile("nop");
    }

    /* Read current count */
    uint32_t current = lapic_read(LAPIC_TIMER_CCR);
    uint32_t elapsed = 0xFFFFFFFF - current;

    /* Estimate ticks per ms (we waited ~10ms) */
    apic_timer_ticks_per_ms = elapsed / 10;

    /* Calculate initial count for desired frequency */
    uint32_t period_ms = 1000 / frequency;
    uint32_t initial_count = apic_timer_ticks_per_ms * period_ms;

    if (initial_count == 0) {
        initial_count = 1;  /* Minimum */
    }

    /* Configure timer for periodic mode */
    lapic_write(LAPIC_LVT_TIMER, LAPIC_TIMER_PERIODIC | APIC_TIMER_VECTOR);
    lapic_write(LAPIC_TIMER_DCR, LAPIC_TIMER_DIV_16);
    lapic_write(LAPIC_TIMER_ICR, initial_count);
}

void apic_timer_stop(void)
{
    /* Mask timer interrupt */
    lapic_write(LAPIC_LVT_TIMER, LAPIC_LVT_MASKED);
    /* Set initial count to 0 */
    lapic_write(LAPIC_TIMER_ICR, 0);
}

/* ============================================================================
 * Interrupt Control
 * ============================================================================ */

void apic_send_eoi(void)
{
    lapic_eoi();
}

void apic_enable_irq(uint32_t irq)
{
    if (irq < 24) {
        /* External IRQ - unmask in I/O APIC */
        uint32_t reg = IOAPIC_REDTBL(irq);
        uint32_t low = ioapic_read(reg);
        low &= ~IOAPIC_MASKED;
        ioapic_write(reg, low);
    }
}

void apic_disable_irq(uint32_t irq)
{
    if (irq < 24) {
        /* External IRQ - mask in I/O APIC */
        uint32_t reg = IOAPIC_REDTBL(irq);
        uint32_t low = ioapic_read(reg);
        low |= IOAPIC_MASKED;
        ioapic_write(reg, low);
    }
}
