/*
 * Gracemont Industrial Control Framework
 * ARM64 RTOS Linker Script
 */

ENTRY(_start)

MEMORY
{
    RAM (rwx)   : ORIGIN = 0x40000000, LENGTH = 256M
    DMA (rwx)   : ORIGIN = 0x50000000, LENGTH = 16M
}

SECTIONS
{
    /* Code */
    .text : {
        . = ALIGN(4096);
        *(.text.boot)
        *(.text*)
        *(.rodata*)
    } > RAM

    /* Exception vectors - must be 2KB aligned */
    .vectors : {
        . = ALIGN(2048);
        KEEP(*(.vectors))
    } > RAM

    /* Page tables - must be 4KB aligned */
    .pagetables : {
        . = ALIGN(4096);
        *(.pagetables)
    } > RAM

    /* Read-only data */
    .rodata : {
        . = ALIGN(8);
        *(.rodata*)
    } > RAM

    /* Initialized data */
    .data : {
        . = ALIGN(8);
        _data_start = .;
        *(.data*)
        _data_end = .;
    } > RAM

    /* BSS */
    .bss : {
        . = ALIGN(8);
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        _bss_end = .;
    } > RAM

    /* Heap */
    .heap : {
        . = ALIGN(16);
        _heap_start = .;
        *(.heap)
        . = . + 16M;
        _heap_end = .;
    } > RAM

    /* Stack */
    .stack : {
        . = ALIGN(16);
        _stack_bottom = .;
        . = . + 64K;
        _stack_top = .;
    } > RAM

    /* DMA/Zero-Copy Pool */
    .dma : {
        . = ALIGN(4096);
        _dma_start = .;
        *(.dma)
        _dma_end = .;
    } > DMA

    /* Zero-Copy Buffer Pool */
    .zbuf : {
        . = ALIGN(64);
        _zbuf_start = .;
        *(.zbuf)
        _zbuf_end = .;
    } > DMA

    /* Symbols for startup code */
    _end = .;
}
