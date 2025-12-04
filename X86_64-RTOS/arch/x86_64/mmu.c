/*
 * Gracemont X86_64 RTOS - MMU/Paging Implementation
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "x86_64/paging.h"
#include "x86_64/cpu.h"
#include "rtos_types.h"

/* ============================================================================
 * Page Tables (set up by boot code, we may extend them here)
 * ============================================================================ */

/* These are defined in multiboot2.asm and used during boot */
extern uint64_t pml4_table[];
extern uint64_t pdpt_table[];
extern uint64_t pd_table[];

/* Additional page tables for dynamic mapping */
static uint64_t kernel_pt_table[512] __attribute__((aligned(4096)));

/* ============================================================================
 * MMU Initialization
 * ============================================================================ */

void mmu_init(void)
{
    /* The boot code already set up identity mapping and higher half mapping.
     * Here we can add additional mappings if needed.
     *
     * Current mapping (from boot):
     * - Identity map: 0x00000000 - 0x7FFFFFFF (first 2GB)
     * - Higher half:  0xFFFF800000000000 - 0xFFFF80007FFFFFFF
     *
     * Both use 2MB huge pages for simplicity.
     */

    /* Enable NX bit if supported */
    uint32_t eax, ebx, ecx, edx;
    cpuid_call(0x80000001, &eax, &ebx, &ecx, &edx);

    if (edx & CPU_FEATURE_NX) {
        uint64_t efer = rdmsr(MSR_EFER);
        efer |= EFER_NXE;
        wrmsr(MSR_EFER, efer);
    }

    /* Enable global pages if supported */
    cpuid_call(0x01, &eax, &ebx, &ecx, &edx);
    if (edx & CPU_FEATURE_PGE) {
        uint64_t cr4 = read_cr4();
        cr4 |= CR4_PGE;
        write_cr4(cr4);
    }

    /* Flush TLB */
    mmu_flush_tlb();
}

/* ============================================================================
 * Page Mapping Functions
 * ============================================================================ */

void mmu_map_page(uint64_t virt, uint64_t phys, uint64_t flags)
{
    /* Get page table indices */
    uint64_t pml4_idx = PML4_INDEX(virt);
    uint64_t pdpt_idx = PDPT_INDEX(virt);
    uint64_t pd_idx = PD_INDEX(virt);
    uint64_t pt_idx = PT_INDEX(virt);

    /* Get or create PDPT */
    uint64_t *pml4 = pml4_table;
    if (!(pml4[pml4_idx] & PTE_PRESENT)) {
        /* Would need to allocate new PDPT - not implemented in this basic version */
        return;
    }

    uint64_t *pdpt = (uint64_t*)(pml4[pml4_idx] & PTE_ADDR_MASK);

    /* Get or create PD */
    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) {
        /* Would need to allocate new PD */
        return;
    }

    /* Check if using huge pages (2MB) */
    if (pdpt[pdpt_idx] & PTE_HUGE) {
        /* Can't map 4KB page in 1GB huge page region */
        return;
    }

    uint64_t *pd = (uint64_t*)(pdpt[pdpt_idx] & PTE_ADDR_MASK);

    /* Check if PD entry is a huge page */
    if (pd[pd_idx] & PTE_HUGE) {
        /* Can't map 4KB page in 2MB huge page region */
        return;
    }

    /* Get or create PT */
    if (!(pd[pd_idx] & PTE_PRESENT)) {
        /* Use our static PT for kernel mappings */
        pd[pd_idx] = ((uint64_t)kernel_pt_table) | PTE_PRESENT | PTE_WRITABLE;
    }

    uint64_t *pt = (uint64_t*)(pd[pd_idx] & PTE_ADDR_MASK);

    /* Map the page */
    pt[pt_idx] = (phys & PTE_ADDR_MASK) | flags | PTE_PRESENT;

    /* Flush TLB for this page */
    mmu_flush_page((void*)virt);
}

void mmu_unmap_page(uint64_t virt)
{
    uint64_t pml4_idx = PML4_INDEX(virt);
    uint64_t pdpt_idx = PDPT_INDEX(virt);
    uint64_t pd_idx = PD_INDEX(virt);
    uint64_t pt_idx = PT_INDEX(virt);

    uint64_t *pml4 = pml4_table;
    if (!(pml4[pml4_idx] & PTE_PRESENT)) return;

    uint64_t *pdpt = (uint64_t*)(pml4[pml4_idx] & PTE_ADDR_MASK);
    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) return;
    if (pdpt[pdpt_idx] & PTE_HUGE) return;

    uint64_t *pd = (uint64_t*)(pdpt[pdpt_idx] & PTE_ADDR_MASK);
    if (!(pd[pd_idx] & PTE_PRESENT)) return;
    if (pd[pd_idx] & PTE_HUGE) return;

    uint64_t *pt = (uint64_t*)(pd[pd_idx] & PTE_ADDR_MASK);

    /* Clear the page table entry */
    pt[pt_idx] = 0;

    /* Flush TLB for this page */
    mmu_flush_page((void*)virt);
}

uint64_t mmu_get_phys(uint64_t virt)
{
    uint64_t pml4_idx = PML4_INDEX(virt);
    uint64_t pdpt_idx = PDPT_INDEX(virt);
    uint64_t pd_idx = PD_INDEX(virt);
    uint64_t pt_idx = PT_INDEX(virt);

    uint64_t *pml4 = pml4_table;
    if (!(pml4[pml4_idx] & PTE_PRESENT)) return 0;

    uint64_t *pdpt = (uint64_t*)(pml4[pml4_idx] & PTE_ADDR_MASK);
    if (!(pdpt[pdpt_idx] & PTE_PRESENT)) return 0;

    /* Check for 1GB huge page */
    if (pdpt[pdpt_idx] & PTE_HUGE) {
        return (pdpt[pdpt_idx] & PTE_ADDR_MASK) + (virt & 0x3FFFFFFF);
    }

    uint64_t *pd = (uint64_t*)(pdpt[pdpt_idx] & PTE_ADDR_MASK);
    if (!(pd[pd_idx] & PTE_PRESENT)) return 0;

    /* Check for 2MB huge page */
    if (pd[pd_idx] & PTE_HUGE) {
        return (pd[pd_idx] & PTE_ADDR_MASK) + (virt & 0x1FFFFF);
    }

    uint64_t *pt = (uint64_t*)(pd[pd_idx] & PTE_ADDR_MASK);
    if (!(pt[pt_idx] & PTE_PRESENT)) return 0;

    return (pt[pt_idx] & PTE_ADDR_MASK) + (virt & 0xFFF);
}

/* ============================================================================
 * TLB Management
 * ============================================================================ */

void mmu_flush_tlb(void)
{
    /* Reload CR3 to flush entire TLB */
    uint64_t cr3 = read_cr3();
    write_cr3(cr3);
}

void mmu_flush_page(void *addr)
{
    invlpg(addr);
}

/* ============================================================================
 * Cache Management
 * ============================================================================ */

void dcache_invalidate(void *addr, size_t size)
{
    /* x86_64 uses CLFLUSH or CLFLUSHOPT */
    uint64_t start = (uint64_t)addr & ~63UL;    /* Cache line aligned */
    uint64_t end = ((uint64_t)addr + size + 63) & ~63UL;

    for (uint64_t a = start; a < end; a += 64) {
        __asm__ volatile("clflush (%0)" : : "r"(a) : "memory");
    }
    __asm__ volatile("mfence" ::: "memory");
}

void dcache_clean(void *addr, size_t size)
{
    /* On x86_64, CLFLUSH both cleans and invalidates */
    dcache_invalidate(addr, size);
}

void dcache_clean_invalidate(void *addr, size_t size)
{
    dcache_invalidate(addr, size);
}
