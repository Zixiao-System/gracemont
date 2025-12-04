/*
 * Gracemont X86_64 RTOS - x86_64 CPU Definitions
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef X86_64_CPU_H
#define X86_64_CPU_H

#include <stdint.h>

/* ============================================================================
 * CPU Feature Flags
 * ============================================================================ */

/* CPUID.01H:EDX */
#define CPU_FEATURE_FPU         (1 << 0)
#define CPU_FEATURE_VME         (1 << 1)
#define CPU_FEATURE_DE          (1 << 2)
#define CPU_FEATURE_PSE         (1 << 3)
#define CPU_FEATURE_TSC         (1 << 4)
#define CPU_FEATURE_MSR         (1 << 5)
#define CPU_FEATURE_PAE         (1 << 6)
#define CPU_FEATURE_MCE         (1 << 7)
#define CPU_FEATURE_CX8         (1 << 8)
#define CPU_FEATURE_APIC        (1 << 9)
#define CPU_FEATURE_SEP         (1 << 11)
#define CPU_FEATURE_MTRR        (1 << 12)
#define CPU_FEATURE_PGE         (1 << 13)
#define CPU_FEATURE_MCA         (1 << 14)
#define CPU_FEATURE_CMOV        (1 << 15)
#define CPU_FEATURE_PAT         (1 << 16)
#define CPU_FEATURE_PSE36       (1 << 17)
#define CPU_FEATURE_CLFLUSH     (1 << 19)
#define CPU_FEATURE_MMX         (1 << 23)
#define CPU_FEATURE_FXSR        (1 << 24)
#define CPU_FEATURE_SSE         (1 << 25)
#define CPU_FEATURE_SSE2        (1 << 26)

/* CPUID.01H:ECX */
#define CPU_FEATURE_SSE3        (1 << 0)
#define CPU_FEATURE_SSSE3       (1 << 9)
#define CPU_FEATURE_SSE41       (1 << 19)
#define CPU_FEATURE_SSE42       (1 << 20)
#define CPU_FEATURE_X2APIC      (1 << 21)
#define CPU_FEATURE_AVX         (1 << 28)

/* CPUID.80000001H:EDX */
#define CPU_FEATURE_SYSCALL     (1 << 11)
#define CPU_FEATURE_NX          (1 << 20)
#define CPU_FEATURE_LM          (1 << 29)    /* Long mode */

/* ============================================================================
 * Control Registers
 * ============================================================================ */

/* CR0 bits */
#define CR0_PE      (1 << 0)    /* Protection Enable */
#define CR0_MP      (1 << 1)    /* Monitor Coprocessor */
#define CR0_EM      (1 << 2)    /* Emulation */
#define CR0_TS      (1 << 3)    /* Task Switched */
#define CR0_ET      (1 << 4)    /* Extension Type */
#define CR0_NE      (1 << 5)    /* Numeric Error */
#define CR0_WP      (1 << 16)   /* Write Protect */
#define CR0_AM      (1 << 18)   /* Alignment Mask */
#define CR0_NW      (1 << 29)   /* Not Write-through */
#define CR0_CD      (1 << 30)   /* Cache Disable */
#define CR0_PG      (1 << 31)   /* Paging */

/* CR4 bits */
#define CR4_VME     (1 << 0)    /* Virtual-8086 Mode Extensions */
#define CR4_PVI     (1 << 1)    /* Protected-mode Virtual Interrupts */
#define CR4_TSD     (1 << 2)    /* Time Stamp Disable */
#define CR4_DE      (1 << 3)    /* Debugging Extensions */
#define CR4_PSE     (1 << 4)    /* Page Size Extension */
#define CR4_PAE     (1 << 5)    /* Physical Address Extension */
#define CR4_MCE     (1 << 6)    /* Machine Check Exception */
#define CR4_PGE     (1 << 7)    /* Page Global Enable */
#define CR4_PCE     (1 << 8)    /* Performance Counter Enable */
#define CR4_OSFXSR  (1 << 9)    /* OS support for FXSAVE/FXRSTOR */
#define CR4_OSXMMEXCPT (1 << 10) /* OS support for unmasked SIMD FP exceptions */
#define CR4_FSGSBASE (1 << 16)  /* Enable RDFSBASE/RDGSBASE/WRFSBASE/WRGSBASE */
#define CR4_OSXSAVE (1 << 18)   /* Enable XSAVE */

/* ============================================================================
 * Model Specific Registers (MSRs)
 * ============================================================================ */

#define MSR_EFER            0xC0000080
#define MSR_STAR            0xC0000081
#define MSR_LSTAR           0xC0000082
#define MSR_CSTAR           0xC0000083
#define MSR_SFMASK          0xC0000084
#define MSR_FS_BASE         0xC0000100
#define MSR_GS_BASE         0xC0000101
#define MSR_KERNEL_GS_BASE  0xC0000102

#define MSR_IA32_APIC_BASE  0x1B

/* EFER bits */
#define EFER_SCE    (1 << 0)    /* System Call Extensions */
#define EFER_LME    (1 << 8)    /* Long Mode Enable */
#define EFER_LMA    (1 << 10)   /* Long Mode Active */
#define EFER_NXE    (1 << 11)   /* No-Execute Enable */

/* ============================================================================
 * RFLAGS bits
 * ============================================================================ */

#define RFLAGS_CF   (1 << 0)    /* Carry Flag */
#define RFLAGS_PF   (1 << 2)    /* Parity Flag */
#define RFLAGS_AF   (1 << 4)    /* Auxiliary Flag */
#define RFLAGS_ZF   (1 << 6)    /* Zero Flag */
#define RFLAGS_SF   (1 << 7)    /* Sign Flag */
#define RFLAGS_TF   (1 << 8)    /* Trap Flag */
#define RFLAGS_IF   (1 << 9)    /* Interrupt Enable Flag */
#define RFLAGS_DF   (1 << 10)   /* Direction Flag */
#define RFLAGS_OF   (1 << 11)   /* Overflow Flag */
#define RFLAGS_IOPL (3 << 12)   /* I/O Privilege Level */
#define RFLAGS_NT   (1 << 14)   /* Nested Task */
#define RFLAGS_RF   (1 << 16)   /* Resume Flag */
#define RFLAGS_VM   (1 << 17)   /* Virtual-8086 Mode */
#define RFLAGS_AC   (1 << 18)   /* Alignment Check */
#define RFLAGS_VIF  (1 << 19)   /* Virtual Interrupt Flag */
#define RFLAGS_VIP  (1 << 20)   /* Virtual Interrupt Pending */
#define RFLAGS_ID   (1 << 21)   /* ID Flag */

/* ============================================================================
 * CPU Functions (implemented in assembly)
 * ============================================================================ */

/* Control register access */
extern uint64_t read_cr0(void);
extern void write_cr0(uint64_t val);
extern uint64_t read_cr2(void);
extern uint64_t read_cr3(void);
extern void write_cr3(uint64_t val);
extern uint64_t read_cr4(void);
extern void write_cr4(uint64_t val);

/* MSR access */
extern uint64_t rdmsr(uint32_t msr);
extern void wrmsr(uint32_t msr, uint64_t value);

/* Timestamp counter */
extern uint64_t rdtsc(void);

/* CPUID */
extern void cpuid_call(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

/* I/O ports */
extern void outb(uint16_t port, uint8_t val);
extern void outw(uint16_t port, uint16_t val);
extern void outl(uint16_t port, uint32_t val);
extern uint8_t inb(uint16_t port);
extern uint16_t inw(uint16_t port);
extern uint32_t inl(uint16_t port);
extern void io_wait(void);

/* Interrupts */
extern void enable_interrupts(void);
extern void disable_interrupts(void);
extern uint64_t read_rflags(void);
extern void write_rflags(uint64_t flags);

/* TLB */
extern void invlpg(void *addr);

/* Descriptor tables */
extern void lgdt(void *gdt_ptr);
extern void lidt(void *idt_ptr);
extern void ltr(uint16_t selector);

/* Halt */
extern void halt(void) __attribute__((noreturn));

/* ============================================================================
 * CPU Info Structure
 * ============================================================================ */

typedef struct {
    char        vendor[13];
    char        brand[49];
    uint32_t    family;
    uint32_t    model;
    uint32_t    stepping;
    uint32_t    features_edx;   /* CPUID.01H:EDX */
    uint32_t    features_ecx;   /* CPUID.01H:ECX */
    uint32_t    ext_features;   /* CPUID.80000001H:EDX */
    uint64_t    tsc_freq;       /* TSC frequency in Hz */
} cpu_info_t;

/* Global CPU info */
extern cpu_info_t cpu_info;

/* Initialize CPU info */
void cpu_detect(void);

#endif /* X86_64_CPU_H */
