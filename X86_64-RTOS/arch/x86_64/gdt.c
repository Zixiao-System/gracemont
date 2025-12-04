/*
 * Gracemont X86_64 RTOS - GDT Implementation
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "x86_64/gdt.h"
#include "x86_64/cpu.h"
#include <stdint.h>

/* ============================================================================
 * GDT Entries
 * ============================================================================ */

/* Number of GDT entries (including TSS which takes 2 slots) */
#define GDT_ENTRIES 7

/* GDT table */
static struct {
    gdt_entry_t entries[5];
    tss_entry_t tss;
} __attribute__((packed, aligned(16))) gdt;

/* GDT pointer */
static gdt_ptr_t gdt_ptr;

/* Task State Segment */
static tss_t tss __attribute__((aligned(16)));

/* Interrupt stack (for IST1) */
static uint8_t interrupt_stack[8192] __attribute__((aligned(16)));

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

static void gdt_set_entry(int num, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t gran)
{
    gdt.entries[num].base_low = base & 0xFFFF;
    gdt.entries[num].base_middle = (base >> 16) & 0xFF;
    gdt.entries[num].base_high = (base >> 24) & 0xFF;
    gdt.entries[num].limit_low = limit & 0xFFFF;
    gdt.entries[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt.entries[num].access = access;
}

static void gdt_set_tss(uint64_t base, uint32_t limit)
{
    gdt.tss.limit_low = limit & 0xFFFF;
    gdt.tss.base_low = base & 0xFFFF;
    gdt.tss.base_middle = (base >> 16) & 0xFF;
    gdt.tss.access = GDT_ACCESS_TSS;
    gdt.tss.granularity = ((limit >> 16) & 0x0F);
    gdt.tss.base_high = (base >> 24) & 0xFF;
    gdt.tss.base_upper = (base >> 32) & 0xFFFFFFFF;
    gdt.tss.reserved = 0;
}

/* ============================================================================
 * GDT Initialization
 * ============================================================================ */

void gdt_init(void)
{
    /* Initialize TSS */
    for (int i = 0; i < (int)sizeof(tss); i++) {
        ((uint8_t*)&tss)[i] = 0;
    }

    /* Set RSP0 (kernel stack for interrupts from ring 3) */
    /* This will be updated when tasks are created */
    extern uint8_t _stack_top[];
    tss.rsp0 = (uint64_t)_stack_top;

    /* Set IST1 for critical interrupts (NMI, Double Fault, etc.) */
    tss.ist1 = (uint64_t)&interrupt_stack[sizeof(interrupt_stack)];

    /* I/O Permission Bitmap offset (set to end of TSS = no IOPB) */
    tss.iopb_offset = sizeof(tss_t);

    /* Set up GDT entries */

    /* Entry 0: Null descriptor (required) */
    gdt_set_entry(0, 0, 0, 0, 0);

    /* Entry 1: Kernel Code Segment (64-bit) */
    gdt_set_entry(1, 0, 0xFFFFF,
                  GDT_ACCESS_PRESENT | GDT_ACCESS_DPL0 | GDT_ACCESS_SEGMENT |
                  GDT_ACCESS_EXECUTABLE | GDT_ACCESS_RW,
                  GDT_GRAN_4K | GDT_GRAN_64BIT);

    /* Entry 2: Kernel Data Segment */
    gdt_set_entry(2, 0, 0xFFFFF,
                  GDT_ACCESS_PRESENT | GDT_ACCESS_DPL0 | GDT_ACCESS_SEGMENT |
                  GDT_ACCESS_RW,
                  GDT_GRAN_4K | GDT_GRAN_32BIT);

    /* Entry 3: User Code Segment (64-bit) */
    gdt_set_entry(3, 0, 0xFFFFF,
                  GDT_ACCESS_PRESENT | GDT_ACCESS_DPL3 | GDT_ACCESS_SEGMENT |
                  GDT_ACCESS_EXECUTABLE | GDT_ACCESS_RW,
                  GDT_GRAN_4K | GDT_GRAN_64BIT);

    /* Entry 4: User Data Segment */
    gdt_set_entry(4, 0, 0xFFFFF,
                  GDT_ACCESS_PRESENT | GDT_ACCESS_DPL3 | GDT_ACCESS_SEGMENT |
                  GDT_ACCESS_RW,
                  GDT_GRAN_4K | GDT_GRAN_32BIT);

    /* Entry 5-6: TSS (takes 2 entries in 64-bit mode) */
    gdt_set_tss((uint64_t)&tss, sizeof(tss_t) - 1);

    /* Load GDT */
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint64_t)&gdt;
    lgdt(&gdt_ptr);

    /* Load TSS */
    ltr(GDT_TSS);
}

/* ============================================================================
 * TSS Functions
 * ============================================================================ */

void tss_set_rsp0(uint64_t rsp0)
{
    tss.rsp0 = rsp0;
}
