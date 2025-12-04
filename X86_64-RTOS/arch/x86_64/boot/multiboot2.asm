;
; Gracemont X86_64 RTOS - Multiboot2 Boot Entry
; Copyright (C) 2024 Zixiao System
;
; SPDX-License-Identifier: GPL-3.0-or-later
;
; This file contains the Multiboot2 header and 32-bit entry point.
; It sets up the environment for transitioning to 64-bit long mode.
;

bits 32

; ============================================================================
; Multiboot2 Header
; ============================================================================

section .multiboot2
align 8

MULTIBOOT2_MAGIC        equ 0xE85250D6
MULTIBOOT2_ARCH_I386    equ 0
MULTIBOOT2_HEADER_LEN   equ multiboot2_header_end - multiboot2_header
MULTIBOOT2_CHECKSUM     equ -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH_I386 + MULTIBOOT2_HEADER_LEN)

multiboot2_header:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH_I386
    dd MULTIBOOT2_HEADER_LEN
    dd MULTIBOOT2_CHECKSUM

    ; Framebuffer tag (optional, for graphical output)
    align 8
    dw 5                        ; type: framebuffer
    dw 0                        ; flags
    dd 20                       ; size
    dd 1024                     ; width
    dd 768                      ; height
    dd 32                       ; depth

    ; End tag (required)
    align 8
    dw 0                        ; type: end
    dw 0                        ; flags
    dd 8                        ; size
multiboot2_header_end:

; ============================================================================
; 32-bit Entry Point
; ============================================================================

section .text.boot
global _start
extern long_mode_start

; Multiboot2 magic number (passed in EAX by bootloader)
MULTIBOOT2_BOOTLOADER_MAGIC equ 0x36D76289

_start:
    ; Disable interrupts
    cli

    ; Save Multiboot2 info pointer (EBX) and magic (EAX) FIRST!
    ; Note: In Multiboot2, EAX=magic, EBX=info
    mov edi, eax                ; Multiboot2 magic
    mov esi, ebx                ; Multiboot2 info struct pointer

    ; === Early Serial Debug ===
    ; Initialize COM1 at 0x3F8
    mov dx, 0x3F8 + 1       ; IER - disable interrupts
    xor al, al
    out dx, al

    mov dx, 0x3F8 + 3       ; LCR - enable DLAB
    mov al, 0x80
    out dx, al

    mov dx, 0x3F8 + 0       ; DLL - baud rate divisor low (115200)
    mov al, 1
    out dx, al

    mov dx, 0x3F8 + 1       ; DLH - baud rate divisor high
    xor al, al
    out dx, al

    mov dx, 0x3F8 + 3       ; LCR - 8N1
    mov al, 0x03
    out dx, al

    mov dx, 0x3F8 + 2       ; FCR - enable FIFO
    mov al, 0xC7
    out dx, al

    mov dx, 0x3F8 + 4       ; MCR - RTS/DSR
    mov al, 0x0B
    out dx, al

    ; Print "BOOT\r\n" to serial
    mov dx, 0x3F8
    mov al, 'B'
    out dx, al
    mov al, 'O'
    out dx, al
    mov al, 'O'
    out dx, al
    mov al, 'T'
    out dx, al
    mov al, 0x0D            ; CR
    out dx, al
    mov al, 0x0A            ; LF
    out dx, al
    ; === End Early Serial Debug ===

    ; Check Multiboot2 magic (now in EDI)
    cmp edi, MULTIBOOT2_BOOTLOADER_MAGIC
    jne .no_multiboot

    ; Print 'M' - magic OK
    mov dx, 0x3F8
    mov al, 'M'
    out dx, al

    ; Set up a temporary stack
    mov esp, stack32_top

    ; Check for CPUID support
    call check_cpuid
    test eax, eax
    jz .no_cpuid

    ; Print 'C' - CPUID OK
    mov dx, 0x3F8
    mov al, 'C'
    out dx, al

    ; Check for long mode support
    call check_long_mode
    test eax, eax
    jz .no_long_mode

    ; Print 'L' - Long mode OK
    mov dx, 0x3F8
    mov al, 'L'
    out dx, al

    ; Set up paging (identity map + higher half)
    call setup_page_tables

    ; Print 'P' - Page tables OK
    mov dx, 0x3F8
    mov al, 'P'
    out dx, al

    call enable_paging

    ; Print 'E' - Paging enabled
    mov dx, 0x3F8
    mov al, 'E'
    out dx, al

    ; EDI/ESI already contain magic and info pointer from the beginning

    ; Print 'G' - About to load GDT
    mov dx, 0x3F8
    mov al, 'G'
    out dx, al

    ; Load 64-bit GDT
    lgdt [gdt64.pointer]

    ; Print 'J' - About to jump
    mov dx, 0x3F8
    mov al, 'J'
    out dx, al
    mov al, 0x0D
    out dx, al
    mov al, 0x0A
    out dx, al

    ; Jump to 64-bit code segment
    jmp gdt64.code:long_mode_start

.no_multiboot:
    ; Print error and actual magic value received
    mov dx, 0x3F8
    mov al, 'E'
    out dx, al
    mov al, 'R'
    out dx, al
    mov al, 'R'
    out dx, al
    mov al, ':'
    out dx, al
    ; Print EDI value (magic we received) as hex
    mov ecx, edi            ; Get the magic value from EDI
    mov ebx, 8              ; 8 hex digits
.print_hex:
    rol ecx, 4              ; Rotate left to get next nibble
    mov al, cl
    and al, 0x0F
    add al, '0'
    cmp al, '9'
    jle .print_digit
    add al, 7               ; 'A' - '9' - 1
.print_digit:
    out dx, al
    dec ebx
    jnz .print_hex
    mov al, 0x0D
    out dx, al
    mov al, 0x0A
    out dx, al
    jmp .error

.no_cpuid:
    mov al, 'C'
    jmp .error

.no_long_mode:
    mov al, 'L'
    jmp .error

.error:
    ; Print error character to VGA
    mov dword [0xB8000], 0x4F524F45  ; "ER" in red
    mov dword [0xB8004], 0x4F3A4F52  ; "R:" in red
    mov byte  [0xB8008], al
    mov byte  [0xB8009], 0x4F        ; Red on white
    hlt
    jmp .error

; ============================================================================
; Check CPUID Support
; ============================================================================

check_cpuid:
    ; Try to flip ID bit (bit 21) in EFLAGS
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    mov eax, 1
    ret
.no_cpuid:
    xor eax, eax
    ret

; ============================================================================
; Check Long Mode Support
; ============================================================================

check_long_mode:
    ; Check for extended CPUID
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    ; Check for long mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29           ; LM bit
    jz .no_long_mode

    mov eax, 1
    ret

.no_long_mode:
    xor eax, eax
    ret

; ============================================================================
; Set Up Page Tables
; ============================================================================

setup_page_tables:
    ; Clear page tables
    mov edi, pml4_table
    mov ecx, 4096 * 4           ; 4 tables * 4096 bytes
    xor eax, eax
    rep stosb

    ; Set up PML4
    mov eax, pdpt_table
    or eax, 0x03                ; Present + Writable
    mov [pml4_table], eax
    ; Also map at entry 256 for higher half (0xFFFF800000000000)
    mov [pml4_table + 256 * 8], eax

    ; Set up PDPT (first 4 entries for 4GB mapping)
    mov eax, pd_table
    or eax, 0x03                ; Present + Writable
    mov [pdpt_table], eax

    ; Second PDPT entry for 1-2GB
    mov eax, pd_table
    add eax, 4096
    or eax, 0x03
    mov [pdpt_table + 8], eax

    ; Set up PD with 2MB pages (identity map first 2GB)
    mov ecx, 512 * 2            ; 512 entries * 2 tables = 2GB
    mov edi, pd_table
    mov eax, 0x83               ; Present + Writable + Huge (2MB page)
.map_pd:
    mov [edi], eax
    add eax, 0x200000           ; Next 2MB
    add edi, 8
    loop .map_pd

    ret

; ============================================================================
; Enable Paging
; ============================================================================

enable_paging:
    ; Load PML4 address into CR3
    mov eax, pml4_table
    mov cr3, eax

    ; Enable PAE (Physical Address Extension)
    mov eax, cr4
    or eax, 1 << 5              ; PAE bit
    mov cr4, eax

    ; Enable Long Mode in EFER MSR
    mov ecx, 0xC0000080         ; EFER MSR
    rdmsr
    or eax, 1 << 8              ; LME bit (Long Mode Enable)
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31             ; PG bit
    mov cr0, eax

    ret

; ============================================================================
; 64-bit GDT
; ============================================================================

section .rodata
align 16

gdt64:
    dq 0                        ; Null descriptor
.code: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)  ; Code segment
.data: equ $ - gdt64
    dq (1 << 44) | (1 << 47) | (1 << 41)              ; Data segment
.pointer:
    dw $ - gdt64 - 1            ; Limit
    dq gdt64                    ; Base

; ============================================================================
; Page Tables (must be page-aligned)
; ============================================================================

section .bss
align 4096

pml4_table:
    resb 4096
pdpt_table:
    resb 4096
pd_table:
    resb 4096 * 2               ; 2 page directories for 2GB

; 32-bit stack
align 16
stack32_bottom:
    resb 4096
stack32_top:
