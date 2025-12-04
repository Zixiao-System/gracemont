/*
 * Gracemont X86_64 RTOS Linker Script
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

ENTRY(_start)

/* Memory layout - Physical addresses only for now */
KERNEL_BASE = 0x100000;        /* 1MB - physical load address */

SECTIONS
{
    . = KERNEL_BASE;

    /* Multiboot2 header must be in first 32KB of kernel */
    .multiboot2 ALIGN(8) : {
        KEEP(*(.multiboot2))
    }

    /* 32-bit boot code and 64-bit trampoline */
    .text.boot ALIGN(16) : {
        *(.text.boot)
        *(.text.trampoline)
    }

    /* Code section */
    .text ALIGN(4K) : {
        _text_start = .;
        *(.text)
        *(.text.*)
        _text_end = .;
    }

    /* Read-only data */
    .rodata ALIGN(4K) : {
        _rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        _rodata_end = .;
    }

    /* Initialized data */
    .data ALIGN(4K) : {
        _data_start = .;
        *(.data)
        *(.data.*)
        _data_end = .;
    }

    /* Uninitialized data (BSS) */
    .bss ALIGN(4K) : {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(16);
        _bss_end = .;
    }

    /* Page tables (must be page-aligned) */
    .page_tables ALIGN(4K) : {
        _page_tables_start = .;
        *(.page_tables)
        . = ALIGN(4K);
        _page_tables_end = .;
    }

    /* Heap */
    .heap ALIGN(16) : {
        _heap_start = .;
        *(.heap)
        . = . + 64K;
        _heap_end = .;
    }

    /* DMA pool */
    .dma ALIGN(4K) : {
        _dma_start = .;
        *(.dma)
        . = . + 16K;
        _dma_end = .;
    }

    /* Zero-copy buffer pool */
    .zbuf ALIGN(64) : {
        _zbuf_start = .;
        *(.zbuf)
        _zbuf_end = .;
    }

    /* Stack (grows downward) */
    .stack ALIGN(16) : {
        _stack_bottom = .;
        . = . + 64K;                /* 64KB kernel stack */
        _stack_top = .;
    }

    /* End of kernel */
    _kernel_end = .;

    /* Debug sections (not loaded) */
    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}

/* Provide symbols */
_kernel_size = _kernel_end - KERNEL_BASE;
