/*
 * Gracemont Industrial Control Framework
 * ARM64 Startup and Exception Vectors
 */

.section .text.boot
.global _start

_start:
    /* Disable interrupts */
    msr     daifset, #0xF

    /* Get CPU ID, hang if not CPU 0 */
    mrs     x0, mpidr_el1
    and     x0, x0, #0xFF
    cbnz    x0, cpu_hang

    /* Setup EL1 if we're at EL2 */
    mrs     x0, CurrentEL
    and     x0, x0, #0xC
    cmp     x0, #0x8
    b.ne    el1_entry

    /* Configure EL2 for EL1 entry */
    mov     x0, #(1 << 31)          /* EL1 is AArch64 */
    msr     hcr_el2, x0

    mov     x0, #0x3C5              /* EL1h, IRQ/FIQ/SError masked */
    msr     spsr_el2, x0

    adr     x0, el1_entry
    msr     elr_el2, x0

    eret

el1_entry:
    /* Setup stack pointer */
    ldr     x0, =_stack_top
    mov     sp, x0

    /* Clear BSS */
    ldr     x0, =_bss_start
    ldr     x1, =_bss_end
bss_loop:
    cmp     x0, x1
    b.ge    bss_done
    str     xzr, [x0], #8
    b       bss_loop
bss_done:

    /* Setup VBAR */
    ldr     x0, =exception_vectors
    msr     vbar_el1, x0

    /* Enable FP/SIMD */
    mov     x0, #(3 << 20)
    msr     cpacr_el1, x0

    /* Configure MMU (identity mapping) */
    bl      mmu_init

    /* Enable caches */
    mrs     x0, sctlr_el1
    orr     x0, x0, #(1 << 2)       /* C - Data cache */
    orr     x0, x0, #(1 << 12)      /* I - Instruction cache */
    msr     sctlr_el1, x0
    isb

    /* Jump to C main */
    bl      kernel_main

    /* Should never return */
cpu_hang:
    wfe
    b       cpu_hang

/*
 * Exception Vectors
 */
.balign 2048
.global exception_vectors
exception_vectors:
    /* Current EL with SP0 */
    .balign 128
    b       sync_sp0
    .balign 128
    b       irq_sp0
    .balign 128
    b       fiq_sp0
    .balign 128
    b       serror_sp0

    /* Current EL with SPx */
    .balign 128
    b       sync_spx
    .balign 128
    b       irq_spx
    .balign 128
    b       fiq_spx
    .balign 128
    b       serror_spx

    /* Lower EL using AArch64 */
    .balign 128
    b       sync_lower64
    .balign 128
    b       irq_lower64
    .balign 128
    b       fiq_lower64
    .balign 128
    b       serror_lower64

    /* Lower EL using AArch32 */
    .balign 128
    b       sync_lower32
    .balign 128
    b       irq_lower32
    .balign 128
    b       fiq_lower32
    .balign 128
    b       serror_lower32

/*
 * Exception Handlers
 */
.macro SAVE_CONTEXT
    sub     sp, sp, #272
    stp     x0, x1, [sp, #0]
    stp     x2, x3, [sp, #16]
    stp     x4, x5, [sp, #32]
    stp     x6, x7, [sp, #48]
    stp     x8, x9, [sp, #64]
    stp     x10, x11, [sp, #80]
    stp     x12, x13, [sp, #96]
    stp     x14, x15, [sp, #112]
    stp     x16, x17, [sp, #128]
    stp     x18, x19, [sp, #144]
    stp     x20, x21, [sp, #160]
    stp     x22, x23, [sp, #176]
    stp     x24, x25, [sp, #192]
    stp     x26, x27, [sp, #208]
    stp     x28, x29, [sp, #224]
    str     x30, [sp, #240]
    mrs     x0, elr_el1
    mrs     x1, spsr_el1
    stp     x0, x1, [sp, #248]
.endm

.macro RESTORE_CONTEXT
    ldp     x0, x1, [sp, #248]
    msr     elr_el1, x0
    msr     spsr_el1, x1
    ldp     x0, x1, [sp, #0]
    ldp     x2, x3, [sp, #16]
    ldp     x4, x5, [sp, #32]
    ldp     x6, x7, [sp, #48]
    ldp     x8, x9, [sp, #64]
    ldp     x10, x11, [sp, #80]
    ldp     x12, x13, [sp, #96]
    ldp     x14, x15, [sp, #112]
    ldp     x16, x17, [sp, #128]
    ldp     x18, x19, [sp, #144]
    ldp     x20, x21, [sp, #160]
    ldp     x22, x23, [sp, #176]
    ldp     x24, x25, [sp, #192]
    ldp     x26, x27, [sp, #208]
    ldp     x28, x29, [sp, #224]
    ldr     x30, [sp, #240]
    add     sp, sp, #272
.endm

sync_sp0:
sync_spx:
sync_lower64:
sync_lower32:
    SAVE_CONTEXT
    mov     x0, sp
    bl      sync_exception_handler
    RESTORE_CONTEXT
    eret

irq_sp0:
irq_spx:
irq_lower64:
irq_lower32:
    SAVE_CONTEXT
    bl      irq_handler
    RESTORE_CONTEXT
    eret

fiq_sp0:
fiq_spx:
fiq_lower64:
fiq_lower32:
    SAVE_CONTEXT
    bl      fiq_handler
    RESTORE_CONTEXT
    eret

serror_sp0:
serror_spx:
serror_lower64:
serror_lower32:
    SAVE_CONTEXT
    mov     x0, sp
    bl      serror_handler
    RESTORE_CONTEXT
    eret

/*
 * Context Switch Implementation
 */
.global arch_context_switch
arch_context_switch:
    /* x0 = prev task, x1 = next task */

    /* Save current context to prev task */
    mov     x2, sp
    str     x2, [x0, #0]            /* Save SP */

    stp     x19, x20, [x0, #8]
    stp     x21, x22, [x0, #24]
    stp     x23, x24, [x0, #40]
    stp     x25, x26, [x0, #56]
    stp     x27, x28, [x0, #72]
    stp     x29, x30, [x0, #88]

    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    stp     x2, x3, [x0, #256]      /* Save ELR, SPSR */

    /* Restore context from next task */
    ldr     x2, [x1, #0]            /* Load SP */
    mov     sp, x2

    ldp     x19, x20, [x1, #8]
    ldp     x21, x22, [x1, #24]
    ldp     x23, x24, [x1, #40]
    ldp     x25, x26, [x1, #56]
    ldp     x27, x28, [x1, #72]
    ldp     x29, x30, [x1, #88]

    ldp     x2, x3, [x1, #256]      /* Load ELR, SPSR */
    msr     elr_el1, x2
    msr     spsr_el1, x3

    ret

.global arch_first_switch
arch_first_switch:
    /* x0 = first task to run */

    /* Load context */
    ldr     x2, [x0, #0]
    mov     sp, x2

    ldp     x19, x20, [x0, #8]
    ldp     x21, x22, [x0, #24]
    ldp     x23, x24, [x0, #40]
    ldp     x25, x26, [x0, #56]
    ldp     x27, x28, [x0, #72]
    ldp     x29, x30, [x0, #88]

    /* Load initial x0, x1 for task entry */
    ldp     x0, x1, [x0, #104]

    /* Enable interrupts and jump to task */
    msr     daifclr, #2
    ret
