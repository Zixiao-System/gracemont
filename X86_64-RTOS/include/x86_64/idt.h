/*
 * Gracemont X86_64 RTOS - IDT Definitions
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef X86_64_IDT_H
#define X86_64_IDT_H

#include <stdint.h>

/* ============================================================================
 * IDT Entry Structure (64-bit)
 * ============================================================================ */

typedef struct {
    uint16_t offset_low;        /* Offset bits 0-15 */
    uint16_t selector;          /* Code segment selector */
    uint8_t  ist;               /* Interrupt Stack Table offset */
    uint8_t  type_attr;         /* Type and attributes */
    uint16_t offset_middle;     /* Offset bits 16-31 */
    uint32_t offset_high;       /* Offset bits 32-63 */
    uint32_t reserved;          /* Reserved, must be 0 */
} __attribute__((packed)) idt_entry_t;

/* IDT Pointer */
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr_t;

/* ============================================================================
 * IDT Gate Types
 * ============================================================================ */

#define IDT_TYPE_INTERRUPT  0x8E    /* P=1, DPL=0, Type=Interrupt Gate */
#define IDT_TYPE_TRAP       0x8F    /* P=1, DPL=0, Type=Trap Gate */
#define IDT_TYPE_INTERRUPT3 0xEE    /* P=1, DPL=3, Type=Interrupt Gate (for int3) */

/* ============================================================================
 * Exception Numbers
 * ============================================================================ */

#define EXCEPTION_DE        0   /* Divide Error */
#define EXCEPTION_DB        1   /* Debug */
#define EXCEPTION_NMI       2   /* Non-Maskable Interrupt */
#define EXCEPTION_BP        3   /* Breakpoint */
#define EXCEPTION_OF        4   /* Overflow */
#define EXCEPTION_BR        5   /* Bound Range Exceeded */
#define EXCEPTION_UD        6   /* Invalid Opcode */
#define EXCEPTION_NM        7   /* Device Not Available */
#define EXCEPTION_DF        8   /* Double Fault */
#define EXCEPTION_TS        10  /* Invalid TSS */
#define EXCEPTION_NP        11  /* Segment Not Present */
#define EXCEPTION_SS        12  /* Stack-Segment Fault */
#define EXCEPTION_GP        13  /* General Protection Fault */
#define EXCEPTION_PF        14  /* Page Fault */
#define EXCEPTION_MF        16  /* x87 FPU Error */
#define EXCEPTION_AC        17  /* Alignment Check */
#define EXCEPTION_MC        18  /* Machine Check */
#define EXCEPTION_XM        19  /* SIMD Floating-Point */
#define EXCEPTION_VE        20  /* Virtualization */
#define EXCEPTION_CP        21  /* Control Protection */

/* ============================================================================
 * IRQ Numbers (remapped to vectors 32-47)
 * ============================================================================ */

#define IRQ_BASE            32
#define IRQ_TIMER           0   /* PIT Timer */
#define IRQ_KEYBOARD        1   /* Keyboard */
#define IRQ_CASCADE         2   /* Cascade for PIC2 */
#define IRQ_COM2            3   /* COM2 */
#define IRQ_COM1            4   /* COM1 */
#define IRQ_LPT2            5   /* LPT2 */
#define IRQ_FLOPPY          6   /* Floppy */
#define IRQ_LPT1            7   /* LPT1 (Spurious) */
#define IRQ_RTC             8   /* RTC */
#define IRQ_ACPI            9   /* ACPI */
#define IRQ_MOUSE           12  /* PS/2 Mouse */
#define IRQ_FPU             13  /* FPU */
#define IRQ_ATA1            14  /* Primary ATA */
#define IRQ_ATA2            15  /* Secondary ATA */

/* APIC IRQs */
#define IRQ_APIC_TIMER      0   /* Local APIC Timer */
#define IRQ_APIC_ERROR      19  /* APIC Error */
#define IRQ_APIC_SPURIOUS   255 /* Spurious */

/* ============================================================================
 * Interrupt Frame
 * ============================================================================ */

typedef struct {
    /* Pushed by our ISR stub */
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, error_code;

    /* Pushed by CPU */
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) interrupt_frame_t;

/* ============================================================================
 * Functions
 * ============================================================================ */

void idt_init(void);
void idt_set_gate(int num, uint64_t handler, uint16_t selector, uint8_t type);

#endif /* X86_64_IDT_H */
