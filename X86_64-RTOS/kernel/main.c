/*
 * Gracemont X86_64 RTOS - Kernel Main Entry
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "rtos_types.h"
#include "x86_64/cpu.h"
#include "x86_64/gdt.h"
#include "x86_64/idt.h"
#include "x86_64/apic.h"
#include "x86_64/paging.h"

/* ============================================================================
 * External Functions
 * ============================================================================ */

/* UART */
extern void uart_init(void);
extern void uart_puts(const char *s);
extern void uart_puthex(uint64_t val);
extern void uart_putdec(uint64_t val);

/* CPU */
extern void cpu_detect(void);
extern cpu_info_t cpu_info;

/* ============================================================================
 * Multiboot2 Definitions
 * ============================================================================ */

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

/* Multiboot2 tag types */
#define MULTIBOOT2_TAG_END          0
#define MULTIBOOT2_TAG_CMDLINE      1
#define MULTIBOOT2_TAG_BOOTLOADER   2
#define MULTIBOOT2_TAG_MODULE       3
#define MULTIBOOT2_TAG_BASIC_MEMINFO 4
#define MULTIBOOT2_TAG_BOOTDEV      5
#define MULTIBOOT2_TAG_MMAP         6
#define MULTIBOOT2_TAG_FRAMEBUFFER  8
#define MULTIBOOT2_TAG_ACPI_OLD     14
#define MULTIBOOT2_TAG_ACPI_NEW     15

typedef struct {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) multiboot2_tag_t;

typedef struct {
    uint32_t total_size;
    uint32_t reserved;
} __attribute__((packed)) multiboot2_info_t;

/* ============================================================================
 * Banner
 * ============================================================================ */

static void print_banner(void)
{
    uart_puts("\n");
    uart_puts("================================================================================\n");
    uart_puts("  ____                                         _     ____ _____ ___  ____  \n");
    uart_puts(" / ___|_ __ __ _  ___ ___ _ __ ___   ___  _ __ | |_  |  _ \\_   _/ _ \\/ ___| \n");
    uart_puts("| |  _| '__/ _` |/ __/ _ \\ '_ ` _ \\ / _ \\| '_ \\| __| | |_) || || | | \\___ \\ \n");
    uart_puts("| |_| | | | (_| | (_|  __/ | | | | | (_) | | | | |_  |  _ < | || |_| |___) |\n");
    uart_puts(" \\____|_|  \\__,_|\\___\\___|_| |_| |_|\\___/|_| |_|\\__| |_| \\_\\|_| \\___/|____/ \n");
    uart_puts("\n");
    uart_puts("  X86_64 Industrial Control RTOS\n");
    uart_puts("  Copyright (C) 2024 Zixiao System\n");
    uart_puts("================================================================================\n");
    uart_puts("\n");
}

/* ============================================================================
 * Multiboot2 Info Parsing
 * ============================================================================ */

static void parse_multiboot2_info(uint64_t magic, uint64_t info_addr)
{
    uart_puts("[BOOT] Parsing Multiboot2 info...\n");

    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        uart_puts("[BOOT] ERROR: Invalid Multiboot2 magic: ");
        uart_puthex(magic);
        uart_puts("\n");
        return;
    }

    multiboot2_info_t *info = (multiboot2_info_t*)info_addr;
    uart_puts("[BOOT] Multiboot2 info size: ");
    uart_putdec(info->total_size);
    uart_puts(" bytes\n");

    /* Parse tags */
    multiboot2_tag_t *tag = (multiboot2_tag_t*)(info_addr + 8);
    while (tag->type != MULTIBOOT2_TAG_END) {
        switch (tag->type) {
            case MULTIBOOT2_TAG_CMDLINE:
                uart_puts("[BOOT] Command line: ");
                uart_puts((const char*)(tag + 1));
                uart_puts("\n");
                break;

            case MULTIBOOT2_TAG_BOOTLOADER:
                uart_puts("[BOOT] Bootloader: ");
                uart_puts((const char*)(tag + 1));
                uart_puts("\n");
                break;

            case MULTIBOOT2_TAG_BASIC_MEMINFO:
                uart_puts("[BOOT] Memory: lower=");
                uart_putdec(*((uint32_t*)(tag + 1)));
                uart_puts("KB, upper=");
                uart_putdec(*((uint32_t*)(tag + 1) + 1));
                uart_puts("KB\n");
                break;

            default:
                break;
        }

        /* Move to next tag (8-byte aligned) */
        uint64_t next = (uint64_t)tag + tag->size;
        next = (next + 7) & ~7UL;
        tag = (multiboot2_tag_t*)next;
    }
}

/* ============================================================================
 * Kernel Main Entry Point
 * ============================================================================ */

void kernel_main(uint64_t magic, uint64_t info)
{
    /* Initialize UART first for debug output */
    uart_init();

    /* Print banner */
    print_banner();

    uart_puts("[INIT] Starting X86_64 RTOS initialization...\n");

    /* Parse Multiboot2 info */
    parse_multiboot2_info(magic, info);

    /* Detect CPU features */
    uart_puts("[INIT] Detecting CPU...\n");
    cpu_detect();
    uart_puts("[INIT] CPU: ");
    uart_puts(cpu_info.vendor);
    uart_puts(" ");
    uart_puts(cpu_info.brand);
    uart_puts("\n");

    /* Initialize GDT and TSS */
    uart_puts("[INIT] Setting up GDT...\n");
    gdt_init();

    /* Initialize IDT */
    uart_puts("[INIT] Setting up IDT...\n");
    idt_init();

    /* Initialize MMU (extend page tables if needed) */
    uart_puts("[INIT] Initializing MMU...\n");
    mmu_init();

    /* Initialize APIC */
    uart_puts("[INIT] Initializing APIC...\n");
    apic_init();
    ioapic_init();

    /* Initialize APIC timer (1000 Hz = 1ms tick) */
    uart_puts("[INIT] Starting APIC timer (1000 Hz)...\n");
    apic_timer_init(1000);

    /* Enable interrupts */
    uart_puts("[INIT] Enabling interrupts...\n");
    enable_interrupts();

    /* Initialization complete */
    uart_puts("\n");
    uart_puts("[INIT] ========================================\n");
    uart_puts("[INIT] X86_64 RTOS initialized successfully!\n");
    uart_puts("[INIT] ========================================\n");
    uart_puts("\n");

    /* TODO: Initialize memory allocator */
    /* TODO: Initialize scheduler */
    /* TODO: Create system tasks */
    /* TODO: Initialize network stack */
    /* TODO: Initialize industrial protocols */

    /* For now, just loop and show we're alive */
    uart_puts("[MAIN] Entering main loop...\n");

    volatile uint64_t counter = 0;
    while (1) {
        /* Simple heartbeat every ~10 million iterations */
        if (++counter % 100000000 == 0) {
            uart_puts("[MAIN] Heartbeat: ");
            uart_putdec(counter / 100000000);
            uart_puts("\n");
        }

        /* Halt until next interrupt */
        __asm__ volatile("hlt");
    }
}
