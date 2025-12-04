/*
 * Gracemont X86_64 RTOS - APIC Definitions
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef X86_64_APIC_H
#define X86_64_APIC_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * Local APIC Register Offsets
 * ============================================================================ */

#define LAPIC_ID            0x020   /* Local APIC ID */
#define LAPIC_VERSION       0x030   /* Local APIC Version */
#define LAPIC_TPR           0x080   /* Task Priority Register */
#define LAPIC_APR           0x090   /* Arbitration Priority */
#define LAPIC_PPR           0x0A0   /* Processor Priority */
#define LAPIC_EOI           0x0B0   /* End of Interrupt */
#define LAPIC_RRD           0x0C0   /* Remote Read */
#define LAPIC_LDR           0x0D0   /* Logical Destination */
#define LAPIC_DFR           0x0E0   /* Destination Format */
#define LAPIC_SVR           0x0F0   /* Spurious Interrupt Vector */
#define LAPIC_ISR           0x100   /* In-Service Register (8 regs) */
#define LAPIC_TMR           0x180   /* Trigger Mode Register (8 regs) */
#define LAPIC_IRR           0x200   /* Interrupt Request Register (8 regs) */
#define LAPIC_ESR           0x280   /* Error Status Register */
#define LAPIC_ICR_LOW       0x300   /* Interrupt Command Register (low) */
#define LAPIC_ICR_HIGH      0x310   /* Interrupt Command Register (high) */
#define LAPIC_LVT_TIMER     0x320   /* LVT Timer Register */
#define LAPIC_LVT_THERMAL   0x330   /* LVT Thermal Sensor */
#define LAPIC_LVT_PERF      0x340   /* LVT Performance Counter */
#define LAPIC_LVT_LINT0     0x350   /* LVT LINT0 */
#define LAPIC_LVT_LINT1     0x360   /* LVT LINT1 */
#define LAPIC_LVT_ERROR     0x370   /* LVT Error */
#define LAPIC_TIMER_ICR     0x380   /* Timer Initial Count */
#define LAPIC_TIMER_CCR     0x390   /* Timer Current Count */
#define LAPIC_TIMER_DCR     0x3E0   /* Timer Divide Configuration */

/* ============================================================================
 * APIC Register Bits
 * ============================================================================ */

/* Spurious Vector Register */
#define LAPIC_SVR_ENABLE    (1 << 8)

/* LVT bits */
#define LAPIC_LVT_MASKED    (1 << 16)
#define LAPIC_LVT_PENDING   (1 << 12)

/* Timer modes */
#define LAPIC_TIMER_ONESHOT     0x00000000
#define LAPIC_TIMER_PERIODIC    0x00020000
#define LAPIC_TIMER_TSC         0x00040000

/* Timer divide values */
#define LAPIC_TIMER_DIV_1       0x0B
#define LAPIC_TIMER_DIV_2       0x00
#define LAPIC_TIMER_DIV_4       0x01
#define LAPIC_TIMER_DIV_8       0x02
#define LAPIC_TIMER_DIV_16      0x03
#define LAPIC_TIMER_DIV_32      0x08
#define LAPIC_TIMER_DIV_64      0x09
#define LAPIC_TIMER_DIV_128     0x0A

/* ============================================================================
 * I/O APIC Register Offsets
 * ============================================================================ */

#define IOAPIC_REGSEL       0x00    /* Register Select */
#define IOAPIC_REGWIN       0x10    /* Register Window */

/* I/O APIC Registers (accessed via REGSEL/REGWIN) */
#define IOAPIC_ID           0x00
#define IOAPIC_VER          0x01
#define IOAPIC_ARB          0x02
#define IOAPIC_REDTBL(n)    (0x10 + (n) * 2)

/* Redirection Table Entry bits */
#define IOAPIC_MASKED       (1UL << 16)
#define IOAPIC_LEVEL        (1UL << 15)
#define IOAPIC_LOW_ACTIVE   (1UL << 13)
#define IOAPIC_LOGICAL      (1UL << 11)

/* ============================================================================
 * Default Addresses
 * ============================================================================ */

#define LAPIC_DEFAULT_BASE  0xFEE00000
#define IOAPIC_DEFAULT_BASE 0xFEC00000

/* ============================================================================
 * IRQ Vectors
 * ============================================================================ */

#define APIC_TIMER_VECTOR   32      /* Timer interrupt vector */
#define APIC_ERROR_VECTOR   51      /* Error interrupt vector */
#define APIC_SPURIOUS_VECTOR 255    /* Spurious interrupt vector */

/* ============================================================================
 * Functions
 * ============================================================================ */

/* Initialization */
void apic_init(void);
void ioapic_init(void);

/* Local APIC */
uint32_t lapic_read(uint32_t reg);
void lapic_write(uint32_t reg, uint32_t val);
void lapic_eoi(void);
uint32_t lapic_get_id(void);

/* I/O APIC */
uint32_t ioapic_read(uint32_t reg);
void ioapic_write(uint32_t reg, uint32_t val);
void ioapic_set_irq(uint8_t irq, uint8_t vector, bool masked);

/* Timer */
void apic_timer_init(uint32_t frequency);
void apic_timer_stop(void);

/* Interrupt control */
void apic_send_eoi(void);
void apic_enable_irq(uint32_t irq);
void apic_disable_irq(uint32_t irq);

#endif /* X86_64_APIC_H */
