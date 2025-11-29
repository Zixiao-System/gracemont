/*
 * Gracemont Industrial Control Framework - ARM64 RTOS
 * Copyright (C) 2024 Zixiao System <https://github.com/Zixiao-System>
 *
 * This file is part of Gracemont Industrial Control Framework.
 * Repository: https://github.com/Zixiao-System/gracemont
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * ARM64 MMU Configuration
 */

#include "rtos_types.h"
#include "rtos_config.h"

/* Page Table Definitions */
#define PAGE_SIZE           4096
#define PAGE_SHIFT          12

/* Translation Table Descriptor Types */
#define TT_TYPE_BLOCK       0x1
#define TT_TYPE_TABLE       0x3
#define TT_TYPE_PAGE        0x3

/* Block/Page Attributes */
#define TT_AF               (1UL << 10)     /* Access flag */
#define TT_SH_INNER         (3UL << 8)      /* Inner shareable */
#define TT_SH_OUTER         (2UL << 8)      /* Outer shareable */
#define TT_AP_RW            (0UL << 6)      /* R/W at EL1 */
#define TT_AP_RO            (2UL << 6)      /* Read-only at EL1 */
#define TT_NS               (1UL << 5)      /* Non-secure */

/* MAIR Index */
#define TT_MAIR_DEV         (0UL << 2)      /* Device memory */
#define TT_MAIR_NC          (1UL << 2)      /* Non-cacheable */
#define TT_MAIR_WT          (2UL << 2)      /* Write-through */
#define TT_MAIR_WB          (3UL << 2)      /* Write-back */

/* Normal memory attributes */
#define TT_ATTR_NORMAL      (TT_AF | TT_SH_INNER | TT_AP_RW | TT_MAIR_WB)
#define TT_ATTR_DEVICE      (TT_AF | TT_AP_RW | TT_MAIR_DEV)
#define TT_ATTR_NC          (TT_AF | TT_SH_INNER | TT_AP_RW | TT_MAIR_NC)

/* Page Tables - 4KB aligned */
static uint64_t l1_table[512] ALIGNED(4096) SECTION(".pagetables");
static uint64_t l2_table[512] ALIGNED(4096) SECTION(".pagetables");
static uint64_t l2_device_table[512] ALIGNED(4096) SECTION(".pagetables");

/*
 * Memory Map Configuration
 *
 * 0x00000000 - 0x3FFFFFFF: RAM (1GB, Normal WB)
 * 0x08000000 - 0x08FFFFFF: GIC (16MB, Device)
 * 0x09000000 - 0x09FFFFFF: UART (16MB, Device)
 * 0x0A000000 - 0x0AFFFFFF: Ethernet (16MB, Device)
 * 0x40000000 - 0x7FFFFFFF: MMIO (1GB, Device)
 */

void mmu_init(void)
{
    /* Clear page tables */
    for (int i = 0; i < 512; i++) {
        l1_table[i] = 0;
        l2_table[i] = 0;
        l2_device_table[i] = 0;
    }

    /* L2 table for first 1GB (RAM at 0x00000000) */
    /* Map 2MB blocks */
    for (int i = 0; i < 512; i++) {
        uint64_t addr = (uint64_t)i << 21;

        if (addr >= 0x08000000 && addr < 0x10000000) {
            /* Device region */
            l2_table[i] = addr | TT_TYPE_BLOCK | TT_ATTR_DEVICE;
        } else {
            /* Normal memory */
            l2_table[i] = addr | TT_TYPE_BLOCK | TT_ATTR_NORMAL;
        }
    }

    /* L2 table for device region (0x40000000 - 0x7FFFFFFF) */
    for (int i = 0; i < 512; i++) {
        uint64_t addr = 0x40000000 + ((uint64_t)i << 21);
        l2_device_table[i] = addr | TT_TYPE_BLOCK | TT_ATTR_DEVICE;
    }

    /* L1 table entries */
    l1_table[0] = (uint64_t)l2_table | TT_TYPE_TABLE;           /* 0x00000000 - 0x3FFFFFFF */
    l1_table[1] = (uint64_t)l2_device_table | TT_TYPE_TABLE;    /* 0x40000000 - 0x7FFFFFFF */

    /* Configure MAIR */
    uint64_t mair = (0x00UL << 0)  |    /* Attr0: Device-nGnRnE */
                    (0x44UL << 8)  |    /* Attr1: Normal Non-cacheable */
                    (0xBBUL << 16) |    /* Attr2: Normal Write-Through */
                    (0xFFUL << 24);     /* Attr3: Normal Write-Back */
    __asm__ volatile("msr mair_el1, %0" : : "r"(mair));

    /* Configure TCR */
    uint64_t tcr = (16UL << 0)  |   /* T0SZ = 16, 48-bit VA */
                   (0UL << 6)   |   /* Reserved */
                   (0UL << 7)   |   /* EPD0 = 0, enable TTBR0 walks */
                   (0UL << 8)   |   /* IRGN0 = 0, Normal WB-WA Inner */
                   (0UL << 10)  |   /* ORGN0 = 0, Normal WB-WA Outer */
                   (3UL << 12)  |   /* SH0 = 3, Inner shareable */
                   (0UL << 14)  |   /* TG0 = 0, 4KB granule */
                   (1UL << 23)  |   /* EPD1 = 1, disable TTBR1 walks */
                   (5UL << 32);     /* IPS = 5, 48-bit PA */
    __asm__ volatile("msr tcr_el1, %0" : : "r"(tcr));

    /* Set TTBR0 */
    __asm__ volatile("msr ttbr0_el1, %0" : : "r"(l1_table));

    /* Ensure all writes complete before enabling MMU */
    dsb();
    isb();

    /* Enable MMU */
    uint64_t sctlr;
    __asm__ volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
    sctlr |= (1 << 0);  /* M - MMU enable */
    __asm__ volatile("msr sctlr_el1, %0" : : "r"(sctlr));

    isb();
}

/*
 * Cache Operations
 */
void dcache_invalidate(void *addr, size_t size)
{
    uint64_t line_size = 64;
    uint64_t start = (uint64_t)addr & ~(line_size - 1);
    uint64_t end = ((uint64_t)addr + size + line_size - 1) & ~(line_size - 1);

    for (uint64_t a = start; a < end; a += line_size) {
        __asm__ volatile("dc ivac, %0" : : "r"(a) : "memory");
    }
    dsb();
}

void dcache_clean(void *addr, size_t size)
{
    uint64_t line_size = 64;
    uint64_t start = (uint64_t)addr & ~(line_size - 1);
    uint64_t end = ((uint64_t)addr + size + line_size - 1) & ~(line_size - 1);

    for (uint64_t a = start; a < end; a += line_size) {
        __asm__ volatile("dc cvac, %0" : : "r"(a) : "memory");
    }
    dsb();
}

void dcache_clean_invalidate(void *addr, size_t size)
{
    uint64_t line_size = 64;
    uint64_t start = (uint64_t)addr & ~(line_size - 1);
    uint64_t end = ((uint64_t)addr + size + line_size - 1) & ~(line_size - 1);

    for (uint64_t a = start; a < end; a += line_size) {
        __asm__ volatile("dc civac, %0" : : "r"(a) : "memory");
    }
    dsb();
}
