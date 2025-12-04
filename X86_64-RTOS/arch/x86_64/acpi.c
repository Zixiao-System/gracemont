/*
 * Gracemont X86_64 RTOS - ACPI Support
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "rtos_types.h"
#include <stdint.h>

/* ============================================================================
 * ACPI Table Signatures
 * ============================================================================ */

#define ACPI_SIG_RSDP   "RSD PTR "
#define ACPI_SIG_RSDT   "RSDT"
#define ACPI_SIG_XSDT   "XSDT"
#define ACPI_SIG_MADT   "APIC"
#define ACPI_SIG_FADT   "FACP"
#define ACPI_SIG_HPET   "HPET"

/* ============================================================================
 * ACPI Structures
 * ============================================================================ */

/* RSDP - Root System Description Pointer */
typedef struct {
    char        signature[8];
    uint8_t     checksum;
    char        oem_id[6];
    uint8_t     revision;
    uint32_t    rsdt_address;
    /* ACPI 2.0+ fields */
    uint32_t    length;
    uint64_t    xsdt_address;
    uint8_t     extended_checksum;
    uint8_t     reserved[3];
} __attribute__((packed)) acpi_rsdp_t;

/* Standard ACPI Table Header */
typedef struct {
    char        signature[4];
    uint32_t    length;
    uint8_t     revision;
    uint8_t     checksum;
    char        oem_id[6];
    char        oem_table_id[8];
    uint32_t    oem_revision;
    uint32_t    creator_id;
    uint32_t    creator_revision;
} __attribute__((packed)) acpi_header_t;

/* MADT - Multiple APIC Description Table */
typedef struct {
    acpi_header_t header;
    uint32_t    local_apic_address;
    uint32_t    flags;
    /* Followed by variable-length entries */
} __attribute__((packed)) acpi_madt_t;

/* MADT Entry Header */
typedef struct {
    uint8_t     type;
    uint8_t     length;
} __attribute__((packed)) madt_entry_header_t;

/* MADT Entry Types */
#define MADT_TYPE_LOCAL_APIC    0
#define MADT_TYPE_IOAPIC        1
#define MADT_TYPE_OVERRIDE      2
#define MADT_TYPE_NMI           3
#define MADT_TYPE_LOCAL_NMI     4

/* Local APIC Entry */
typedef struct {
    madt_entry_header_t header;
    uint8_t     acpi_processor_id;
    uint8_t     apic_id;
    uint32_t    flags;
} __attribute__((packed)) madt_local_apic_t;

/* I/O APIC Entry */
typedef struct {
    madt_entry_header_t header;
    uint8_t     ioapic_id;
    uint8_t     reserved;
    uint32_t    ioapic_address;
    uint32_t    global_irq_base;
} __attribute__((packed)) madt_ioapic_t;

/* ============================================================================
 * Global ACPI Data
 * ============================================================================ */

static acpi_rsdp_t *rsdp = NULL;
static acpi_madt_t *madt = NULL;

static uint32_t local_apic_address = 0;
static uint32_t ioapic_address = 0;
static uint32_t num_cpus = 0;

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

static int memcmp_simple(const void *s1, const void *s2, size_t n)
{
    const uint8_t *p1 = s1, *p2 = s2;
    while (n--) {
        if (*p1++ != *p2++) {
            return p1[-1] - p2[-1];
        }
    }
    return 0;
}

static uint8_t acpi_checksum(void *ptr, size_t len)
{
    uint8_t sum = 0;
    uint8_t *p = ptr;
    while (len--) {
        sum += *p++;
    }
    return sum;
}

/* ============================================================================
 * ACPI Functions
 * ============================================================================ */

/*
 * Find RSDP in memory
 */
static acpi_rsdp_t *acpi_find_rsdp(void)
{
    /* Search EBDA (Extended BIOS Data Area) */
    /* EBDA pointer is at 0x40E */
    uint16_t ebda_seg = *(uint16_t*)0x40E;
    uint64_t ebda = (uint64_t)ebda_seg << 4;

    /* Search first 1KB of EBDA */
    for (uint64_t addr = ebda; addr < ebda + 0x400; addr += 16) {
        if (memcmp_simple((void*)addr, ACPI_SIG_RSDP, 8) == 0) {
            acpi_rsdp_t *rsdp = (acpi_rsdp_t*)addr;
            if (acpi_checksum(rsdp, 20) == 0) {
                return rsdp;
            }
        }
    }

    /* Search BIOS ROM area (0xE0000 - 0xFFFFF) */
    for (uint64_t addr = 0xE0000; addr < 0x100000; addr += 16) {
        if (memcmp_simple((void*)addr, ACPI_SIG_RSDP, 8) == 0) {
            acpi_rsdp_t *rsdp = (acpi_rsdp_t*)addr;
            if (acpi_checksum(rsdp, 20) == 0) {
                return rsdp;
            }
        }
    }

    return NULL;
}

/*
 * Find ACPI table by signature
 */
static void *acpi_find_table(const char *signature)
{
    if (rsdp == NULL) {
        rsdp = acpi_find_rsdp();
        if (rsdp == NULL) return NULL;
    }

    /* Use XSDT if available (ACPI 2.0+) */
    if (rsdp->revision >= 2 && rsdp->xsdt_address) {
        acpi_header_t *xsdt = (acpi_header_t*)rsdp->xsdt_address;
        if (memcmp_simple(xsdt->signature, "XSDT", 4) == 0) {
            int entries = (xsdt->length - sizeof(acpi_header_t)) / 8;
            uint64_t *ptrs = (uint64_t*)(xsdt + 1);

            for (int i = 0; i < entries; i++) {
                acpi_header_t *hdr = (acpi_header_t*)ptrs[i];
                if (memcmp_simple(hdr->signature, signature, 4) == 0) {
                    return hdr;
                }
            }
        }
    }

    /* Fall back to RSDT (32-bit) */
    if (rsdp->rsdt_address) {
        acpi_header_t *rsdt = (acpi_header_t*)(uint64_t)rsdp->rsdt_address;
        if (memcmp_simple(rsdt->signature, "RSDT", 4) == 0) {
            int entries = (rsdt->length - sizeof(acpi_header_t)) / 4;
            uint32_t *ptrs = (uint32_t*)(rsdt + 1);

            for (int i = 0; i < entries; i++) {
                acpi_header_t *hdr = (acpi_header_t*)(uint64_t)ptrs[i];
                if (memcmp_simple(hdr->signature, signature, 4) == 0) {
                    return hdr;
                }
            }
        }
    }

    return NULL;
}

/*
 * Parse MADT for APIC information
 */
static void acpi_parse_madt(void)
{
    madt = (acpi_madt_t*)acpi_find_table(ACPI_SIG_MADT);
    if (madt == NULL) return;

    local_apic_address = madt->local_apic_address;

    /* Parse MADT entries */
    uint8_t *ptr = (uint8_t*)(madt + 1);
    uint8_t *end = (uint8_t*)madt + madt->header.length;

    while (ptr < end) {
        madt_entry_header_t *entry = (madt_entry_header_t*)ptr;

        switch (entry->type) {
            case MADT_TYPE_LOCAL_APIC: {
                madt_local_apic_t *lapic = (madt_local_apic_t*)entry;
                if (lapic->flags & 1) {  /* Processor enabled */
                    num_cpus++;
                }
                break;
            }

            case MADT_TYPE_IOAPIC: {
                madt_ioapic_t *ioapic = (madt_ioapic_t*)entry;
                if (ioapic_address == 0) {
                    ioapic_address = ioapic->ioapic_address;
                }
                break;
            }

            default:
                break;
        }

        ptr += entry->length;
        if (entry->length == 0) break;  /* Safety check */
    }
}

/*
 * Initialize ACPI
 */
void acpi_init(void)
{
    rsdp = acpi_find_rsdp();

    if (rsdp) {
        acpi_parse_madt();
    }

    /* Use defaults if ACPI not available */
    if (local_apic_address == 0) {
        local_apic_address = 0xFEE00000;
    }
    if (ioapic_address == 0) {
        ioapic_address = 0xFEC00000;
    }
    if (num_cpus == 0) {
        num_cpus = 1;
    }
}

/*
 * Get APIC addresses
 */
uint32_t acpi_get_lapic_address(void)
{
    return local_apic_address;
}

uint32_t acpi_get_ioapic_address(void)
{
    return ioapic_address;
}

uint32_t acpi_get_num_cpus(void)
{
    return num_cpus;
}
