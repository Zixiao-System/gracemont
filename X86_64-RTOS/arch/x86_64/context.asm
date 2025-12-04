;
; Gracemont X86_64 RTOS - Context Switch
; Copyright (C) 2024 Zixiao System
;
; SPDX-License-Identifier: GPL-3.0-or-later
;
; This file implements task context switching for x86_64.
;

bits 64

section .text

; ============================================================================
; Context Switch
; ============================================================================

; void arch_context_switch(tcb_t *prev, tcb_t *next)
;
; TCB layout (must match rtos_types.h):
;   offset 0:  sp   (stack pointer)
;   offset 8:  rbx  (callee-saved)
;   offset 16: rbp
;   offset 24: r12
;   offset 32: r13
;   offset 40: r14
;   offset 48: r15
;   offset 56: rip  (return address)
;
global arch_context_switch
arch_context_switch:
    ; Save callee-saved registers to prev task's TCB

    ; Push return address onto prev's stack
    mov rax, [rsp]              ; Get return address
    mov [rdi + 56], rax         ; Save RIP

    ; Save callee-saved registers
    mov [rdi + 8], rbx
    mov [rdi + 16], rbp
    mov [rdi + 24], r12
    mov [rdi + 32], r13
    mov [rdi + 40], r14
    mov [rdi + 48], r15
    mov [rdi + 0], rsp          ; Save stack pointer

    ; Restore callee-saved registers from next task's TCB
    mov rsp, [rsi + 0]          ; Restore stack pointer
    mov rbx, [rsi + 8]
    mov rbp, [rsi + 16]
    mov r12, [rsi + 24]
    mov r13, [rsi + 32]
    mov r14, [rsi + 40]
    mov r15, [rsi + 48]

    ; Jump to next task's saved RIP
    jmp [rsi + 56]

; ============================================================================
; First Context Switch (start a new task)
; ============================================================================

; void arch_first_switch(tcb_t *task)
;
; This function starts a new task for the first time.
; The task's stack should be set up with:
;   - Task entry point
;   - Initial register values (can be zero)
;
global arch_first_switch
arch_first_switch:
    ; Load task's stack pointer
    mov rsp, [rdi + 0]

    ; Load callee-saved registers
    mov rbx, [rdi + 8]
    mov rbp, [rdi + 16]
    mov r12, [rdi + 24]
    mov r13, [rdi + 32]
    mov r14, [rdi + 40]
    mov r15, [rdi + 48]

    ; Enable interrupts
    sti

    ; Jump to task entry point
    jmp [rdi + 56]

; ============================================================================
; Task Entry Trampoline
; ============================================================================

; void task_entry_trampoline(void)
;
; This is the initial entry point for all tasks.
; It pops the task function and argument from the stack and calls the function.
;
global task_entry_trampoline
task_entry_trampoline:
    ; At this point, stack contains:
    ;   [rsp]     = task function pointer
    ;   [rsp+8]   = task argument
    ;   [rsp+16]  = task_exit function (cleanup)

    pop rax                     ; Task function
    pop rdi                     ; Task argument

    ; Call the task function
    call rax

    ; Task returned - call exit handler
    pop rax                     ; task_exit function
    call rax

    ; Should never reach here
    cli
    hlt
    jmp $

; ============================================================================
; Interrupt State Management
; ============================================================================

; uint64_t arch_irq_save(void)
;
; Disable interrupts and return previous interrupt state
;
global arch_irq_save
arch_irq_save:
    pushfq
    pop rax
    cli
    ret

; void arch_irq_restore(uint64_t flags)
;
; Restore interrupt state
;
global arch_irq_restore
arch_irq_restore:
    push rdi
    popfq
    ret

; bool arch_irq_enabled(void)
;
; Check if interrupts are enabled
;
global arch_irq_enabled
arch_irq_enabled:
    pushfq
    pop rax
    and rax, 0x200              ; IF flag
    shr rax, 9
    ret
