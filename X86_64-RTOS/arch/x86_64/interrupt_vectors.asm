;
; Gracemont X86_64 RTOS - Interrupt Vectors
; Copyright (C) 2024 Zixiao System
;
; SPDX-License-Identifier: GPL-3.0-or-later
;
; This file defines all interrupt service routine entry points.
;

bits 64

; ============================================================================
; External C Handlers
; ============================================================================

extern exception_handler
extern irq_handler

; ============================================================================
; ISR Macros
; ============================================================================

; ISR without error code - push dummy error code
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push 0                      ; Dummy error code
    push %1                     ; Interrupt number
    jmp isr_common_stub
%endmacro

; ISR with error code - CPU already pushed it
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push %1                     ; Interrupt number
    jmp isr_common_stub
%endmacro

; ============================================================================
; Exception Handlers (0-31)
; ============================================================================

ISR_NOERRCODE 0                 ; Divide Error
ISR_NOERRCODE 1                 ; Debug
ISR_NOERRCODE 2                 ; NMI
ISR_NOERRCODE 3                 ; Breakpoint
ISR_NOERRCODE 4                 ; Overflow
ISR_NOERRCODE 5                 ; Bound Range Exceeded
ISR_NOERRCODE 6                 ; Invalid Opcode
ISR_NOERRCODE 7                 ; Device Not Available
ISR_ERRCODE   8                 ; Double Fault
ISR_NOERRCODE 9                 ; Coprocessor Segment Overrun (legacy)
ISR_ERRCODE   10                ; Invalid TSS
ISR_ERRCODE   11                ; Segment Not Present
ISR_ERRCODE   12                ; Stack-Segment Fault
ISR_ERRCODE   13                ; General Protection Fault
ISR_ERRCODE   14                ; Page Fault
ISR_NOERRCODE 15                ; Reserved
ISR_NOERRCODE 16                ; x87 FPU Error
ISR_ERRCODE   17                ; Alignment Check
ISR_NOERRCODE 18                ; Machine Check
ISR_NOERRCODE 19                ; SIMD Floating-Point
ISR_NOERRCODE 20                ; Virtualization
ISR_ERRCODE   21                ; Control Protection
ISR_NOERRCODE 22                ; Reserved
ISR_NOERRCODE 23                ; Reserved
ISR_NOERRCODE 24                ; Reserved
ISR_NOERRCODE 25                ; Reserved
ISR_NOERRCODE 26                ; Reserved
ISR_NOERRCODE 27                ; Reserved
ISR_NOERRCODE 28                ; Reserved
ISR_NOERRCODE 29                ; Reserved
ISR_NOERRCODE 30                ; Reserved
ISR_NOERRCODE 31                ; Reserved

; ============================================================================
; IRQ Handlers (32-47)
; ============================================================================

ISR_NOERRCODE 32                ; IRQ 0 - Timer
ISR_NOERRCODE 33                ; IRQ 1 - Keyboard
ISR_NOERRCODE 34                ; IRQ 2 - Cascade
ISR_NOERRCODE 35                ; IRQ 3 - COM2
ISR_NOERRCODE 36                ; IRQ 4 - COM1
ISR_NOERRCODE 37                ; IRQ 5 - LPT2
ISR_NOERRCODE 38                ; IRQ 6 - Floppy
ISR_NOERRCODE 39                ; IRQ 7 - LPT1/Spurious
ISR_NOERRCODE 40                ; IRQ 8 - RTC
ISR_NOERRCODE 41                ; IRQ 9 - ACPI
ISR_NOERRCODE 42                ; IRQ 10
ISR_NOERRCODE 43                ; IRQ 11
ISR_NOERRCODE 44                ; IRQ 12 - PS/2 Mouse
ISR_NOERRCODE 45                ; IRQ 13 - FPU
ISR_NOERRCODE 46                ; IRQ 14 - Primary ATA
ISR_NOERRCODE 47                ; IRQ 15 - Secondary ATA

; Additional vectors for APIC
ISR_NOERRCODE 255               ; APIC Spurious

; ============================================================================
; Common ISR Stub
; ============================================================================

section .text

isr_common_stub:
    ; Save all general purpose registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Save segment registers (in case we need them)
    mov ax, ds
    push rax

    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax

    ; Pass pointer to interrupt frame as argument
    mov rdi, rsp

    ; Check if this is an exception (0-31) or IRQ (32+)
    mov rax, [rsp + 136]        ; Get interrupt number (offset adjusted for push rax above)
    cmp rax, 32
    jl .exception

    ; Call IRQ handler
    call irq_handler
    jmp .done

.exception:
    ; Call exception handler
    call exception_handler

.done:
    ; Restore segment registers
    pop rax
    mov ds, ax
    mov es, ax

    ; Restore general purpose registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Remove interrupt number and error code
    add rsp, 16

    ; Return from interrupt
    iretq
