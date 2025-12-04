;
; Gracemont X86_64 RTOS - Long Mode Entry
; Copyright (C) 2024 Zixiao System
;
; SPDX-License-Identifier: GPL-3.0-or-later
;
; This file contains the 64-bit long mode entry point.
; It completes system initialization and jumps to C code.
;

bits 64

; ============================================================================
; External Symbols
; ============================================================================

extern kernel_main
extern _bss_start
extern _bss_end
extern _stack_top

; ============================================================================
; 64-bit Entry Point
; ============================================================================

section .text
global long_mode_start

long_mode_start:
    ; === 64-bit mode entered ===
    mov dx, 0x3F8
    mov al, '6'
    out dx, al
    mov al, '4'
    out dx, al
    mov al, 0x0D
    out dx, al
    mov al, 0x0A
    out dx, al

    ; Reload segment registers with 64-bit data segment
    mov ax, 0x10                ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Set up the 64-bit stack
    mov rsp, _stack_top

    ; Save Multiboot2 info (passed in EDI and ESI from 32-bit code)
    ; EDI = magic, ESI = info pointer
    push rdi
    push rsi

    ; NOTE: BSS clear disabled - GRUB/loader should have zeroed it
    ; TODO: Debug page table setup if BSS clear is needed

    ; Restore Multiboot2 info as arguments for kernel_main
    pop rsi                     ; Second argument: info pointer
    pop rdi                     ; First argument: magic

    ; Ensure stack is 16-byte aligned before call
    and rsp, ~0xF
    sub rsp, 8

    ; Call kernel main
    call kernel_main

    ; If kernel_main returns, halt
.halt:
    cli
    hlt
    jmp .halt

; ============================================================================
; Helper Functions
; ============================================================================

; void halt(void) - Halt the CPU
global halt
halt:
    cli
    hlt
    jmp halt

; void enable_interrupts(void)
global enable_interrupts
enable_interrupts:
    sti
    ret

; void disable_interrupts(void)
global disable_interrupts
disable_interrupts:
    cli
    ret

; uint64_t read_rflags(void)
global read_rflags
read_rflags:
    pushfq
    pop rax
    ret

; void write_rflags(uint64_t flags)
global write_rflags
write_rflags:
    push rdi
    popfq
    ret

; uint64_t read_cr0(void)
global read_cr0
read_cr0:
    mov rax, cr0
    ret

; void write_cr0(uint64_t val)
global write_cr0
write_cr0:
    mov cr0, rdi
    ret

; uint64_t read_cr2(void)
global read_cr2
read_cr2:
    mov rax, cr2
    ret

; uint64_t read_cr3(void)
global read_cr3
read_cr3:
    mov rax, cr3
    ret

; void write_cr3(uint64_t val)
global write_cr3
write_cr3:
    mov cr3, rdi
    ret

; uint64_t read_cr4(void)
global read_cr4
read_cr4:
    mov rax, cr4
    ret

; void write_cr4(uint64_t val)
global write_cr4
write_cr4:
    mov cr4, rdi
    ret

; uint64_t rdmsr(uint32_t msr)
global rdmsr
rdmsr:
    mov ecx, edi
    rdmsr
    shl rdx, 32
    or rax, rdx
    ret

; void wrmsr(uint32_t msr, uint64_t value)
global wrmsr
wrmsr:
    mov ecx, edi
    mov eax, esi
    mov rdx, rsi
    shr rdx, 32
    wrmsr
    ret

; uint64_t rdtsc(void)
global rdtsc
rdtsc:
    rdtsc
    shl rdx, 32
    or rax, rdx
    ret

; void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
global cpuid_call
cpuid_call:
    push rbx
    mov eax, edi
    cpuid
    mov [rsi], eax
    mov [rdx], ebx
    mov [rcx], ecx
    mov [r8], edx
    pop rbx
    ret

; void outb(uint16_t port, uint8_t val)
global outb
outb:
    mov dx, di
    mov al, sil
    out dx, al
    ret

; void outw(uint16_t port, uint16_t val)
global outw
outw:
    mov dx, di
    mov ax, si
    out dx, ax
    ret

; void outl(uint16_t port, uint32_t val)
global outl
outl:
    mov dx, di
    mov eax, esi
    out dx, eax
    ret

; uint8_t inb(uint16_t port)
global inb
inb:
    mov dx, di
    in al, dx
    movzx eax, al
    ret

; uint16_t inw(uint16_t port)
global inw
inw:
    mov dx, di
    in ax, dx
    movzx eax, ax
    ret

; uint32_t inl(uint16_t port)
global inl
inl:
    mov dx, di
    in eax, dx
    ret

; void io_wait(void)
global io_wait
io_wait:
    mov dx, 0x80
    mov al, 0
    out dx, al
    ret

; void invlpg(void *addr)
global invlpg
invlpg:
    invlpg [rdi]
    ret

; void lgdt(void *gdt_ptr)
global lgdt
lgdt:
    lgdt [rdi]
    ret

; void lidt(void *idt_ptr)
global lidt
lidt:
    lidt [rdi]
    ret

; void ltr(uint16_t selector)
global ltr
ltr:
    ltr di
    ret
