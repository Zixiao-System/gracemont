/*
 * Gracemont X86_64 RTOS - Paging Definitions
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef X86_64_PAGING_H
#define X86_64_PAGING_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================================
 * Page Sizes
 * ============================================================================ */

#define PAGE_SIZE_4K    0x1000          /* 4 KB */
#define PAGE_SIZE_2M    0x200000        /* 2 MB */
#define PAGE_SIZE_1G    0x40000000      /* 1 GB */

#define PAGE_SIZE       PAGE_SIZE_4K

/* ============================================================================
 * Page Table Entry Flags
 * ============================================================================ */

#define PTE_PRESENT     (1UL << 0)      /* Page is present */
#define PTE_WRITABLE    (1UL << 1)      /* Page is writable */
#define PTE_USER        (1UL << 2)      /* User accessible */
#define PTE_PWT         (1UL << 3)      /* Page-level write-through */
#define PTE_PCD         (1UL << 4)      /* Page-level cache disable */
#define PTE_ACCESSED    (1UL << 5)      /* Page was accessed */
#define PTE_DIRTY       (1UL << 6)      /* Page was written */
#define PTE_HUGE        (1UL << 7)      /* Huge page (2MB or 1GB) */
#define PTE_GLOBAL      (1UL << 8)      /* Global page */
#define PTE_NO_EXEC     (1UL << 63)     /* No execute */

/* Address mask (bits 12-51 for 4KB pages) */
#define PTE_ADDR_MASK   0x000FFFFFFFFFF000UL

/* ============================================================================
 * Virtual Address Structure (48-bit)
 * ============================================================================ */

/*
 * 48-bit virtual address breakdown:
 *
 * 63       48 47       39 38       30 29       21 20       12 11        0
 * +---------+-----------+-----------+-----------+-----------+-----------+
 * |  Sign   |   PML4    |   PDPT    |    PD     |    PT     |  Offset   |
 * | Extend  |  Index    |   Index   |   Index   |   Index   |           |
 * +---------+-----------+-----------+-----------+-----------+-----------+
 *    16 bits   9 bits      9 bits      9 bits      9 bits     12 bits
 */

#define VA_PML4_SHIFT   39
#define VA_PDPT_SHIFT   30
#define VA_PD_SHIFT     21
#define VA_PT_SHIFT     12

#define VA_INDEX_MASK   0x1FF   /* 9 bits */

#define PML4_INDEX(va)  (((va) >> VA_PML4_SHIFT) & VA_INDEX_MASK)
#define PDPT_INDEX(va)  (((va) >> VA_PDPT_SHIFT) & VA_INDEX_MASK)
#define PD_INDEX(va)    (((va) >> VA_PD_SHIFT) & VA_INDEX_MASK)
#define PT_INDEX(va)    (((va) >> VA_PT_SHIFT) & VA_INDEX_MASK)

/* ============================================================================
 * Memory Regions
 * ============================================================================ */

#define KERNEL_PHYS_BASE    0x100000UL              /* 1 MB */
#define KERNEL_VIRT_BASE    0xFFFF800000000000UL    /* Higher half */

/* Convert between physical and virtual addresses */
#define PHYS_TO_VIRT(p)     ((void*)((uint64_t)(p) + KERNEL_VIRT_BASE))
#define VIRT_TO_PHYS(v)     ((uint64_t)(v) - KERNEL_VIRT_BASE)

/* ============================================================================
 * Functions
 * ============================================================================ */

void mmu_init(void);
void mmu_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
void mmu_unmap_page(uint64_t virt);
uint64_t mmu_get_phys(uint64_t virt);
void mmu_flush_tlb(void);
void mmu_flush_page(void *addr);

/* Cache operations */
void dcache_invalidate(void *addr, size_t size);
void dcache_clean(void *addr, size_t size);
void dcache_clean_invalidate(void *addr, size_t size);

#endif /* X86_64_PAGING_H */
