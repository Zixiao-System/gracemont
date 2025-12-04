/*
 * Gracemont X86_64 RTOS - GDT Definitions
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef X86_64_GDT_H
#define X86_64_GDT_H

#include <stdint.h>

/* ============================================================================
 * GDT Segment Selectors
 * ============================================================================ */

#define GDT_NULL        0x00
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE   0x18
#define GDT_USER_DATA   0x20
#define GDT_TSS         0x28

/* ============================================================================
 * GDT Entry Structure
 * ============================================================================ */

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

/* TSS entry is 16 bytes in 64-bit mode */
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed)) tss_entry_t;

/* GDT Pointer */
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_ptr_t;

/* ============================================================================
 * Task State Segment (TSS)
 * ============================================================================ */

typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;          /* Stack pointer for ring 0 */
    uint64_t rsp1;          /* Stack pointer for ring 1 */
    uint64_t rsp2;          /* Stack pointer for ring 2 */
    uint64_t reserved1;
    uint64_t ist1;          /* Interrupt Stack Table 1 */
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;   /* I/O Permission Bitmap offset */
} __attribute__((packed)) tss_t;

/* ============================================================================
 * GDT Access Byte Flags
 * ============================================================================ */

#define GDT_ACCESS_PRESENT      (1 << 7)
#define GDT_ACCESS_DPL0         (0 << 5)
#define GDT_ACCESS_DPL3         (3 << 5)
#define GDT_ACCESS_SEGMENT      (1 << 4)
#define GDT_ACCESS_EXECUTABLE   (1 << 3)
#define GDT_ACCESS_DC           (1 << 2)    /* Direction/Conforming */
#define GDT_ACCESS_RW           (1 << 1)    /* Readable/Writable */
#define GDT_ACCESS_ACCESSED     (1 << 0)

#define GDT_ACCESS_TSS          0x89        /* 64-bit TSS (Available) */

/* ============================================================================
 * GDT Granularity Byte Flags
 * ============================================================================ */

#define GDT_GRAN_4K             (1 << 7)
#define GDT_GRAN_32BIT          (1 << 6)
#define GDT_GRAN_64BIT          (1 << 5)

/* ============================================================================
 * Functions
 * ============================================================================ */

void gdt_init(void);
void tss_set_rsp0(uint64_t rsp0);

#endif /* X86_64_GDT_H */
