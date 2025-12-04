/*
 * Gracemont X86_64 RTOS - CPU Detection
 * Copyright (C) 2024 Zixiao System
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "x86_64/cpu.h"
#include <stdint.h>

/* ============================================================================
 * Global CPU Info
 * ============================================================================ */

cpu_info_t cpu_info;

/* ============================================================================
 * Helper to copy string
 * ============================================================================ */

static void str_copy(char *dst, const char *src, int len)
{
    for (int i = 0; i < len; i++) {
        dst[i] = src[i];
    }
}

/* ============================================================================
 * CPU Detection
 * ============================================================================ */

void cpu_detect(void)
{
    uint32_t eax, ebx, ecx, edx;

    /* Get vendor string */
    cpuid_call(0, &eax, &ebx, &ecx, &edx);

    /* Vendor string is in EBX:EDX:ECX */
    *((uint32_t*)&cpu_info.vendor[0]) = ebx;
    *((uint32_t*)&cpu_info.vendor[4]) = edx;
    *((uint32_t*)&cpu_info.vendor[8]) = ecx;
    cpu_info.vendor[12] = '\0';

    /* Get CPU features */
    if (eax >= 1) {
        cpuid_call(1, &eax, &ebx, &ecx, &edx);

        cpu_info.stepping = eax & 0xF;
        cpu_info.model = (eax >> 4) & 0xF;
        cpu_info.family = (eax >> 8) & 0xF;

        /* Extended model/family for newer CPUs */
        if (cpu_info.family == 0xF) {
            cpu_info.family += (eax >> 20) & 0xFF;
        }
        if (cpu_info.family >= 6) {
            cpu_info.model += ((eax >> 16) & 0xF) << 4;
        }

        cpu_info.features_edx = edx;
        cpu_info.features_ecx = ecx;
    }

    /* Get extended features */
    cpuid_call(0x80000000, &eax, &ebx, &ecx, &edx);

    if (eax >= 0x80000001) {
        cpuid_call(0x80000001, &eax, &ebx, &ecx, &edx);
        cpu_info.ext_features = edx;
    }

    /* Get brand string if available */
    cpuid_call(0x80000000, &eax, &ebx, &ecx, &edx);

    if (eax >= 0x80000004) {
        uint32_t *brand = (uint32_t*)cpu_info.brand;

        cpuid_call(0x80000002, &brand[0], &brand[1], &brand[2], &brand[3]);
        cpuid_call(0x80000003, &brand[4], &brand[5], &brand[6], &brand[7]);
        cpuid_call(0x80000004, &brand[8], &brand[9], &brand[10], &brand[11]);
        cpu_info.brand[48] = '\0';

        /* Trim leading spaces */
        char *p = cpu_info.brand;
        while (*p == ' ') p++;
        if (p != cpu_info.brand) {
            char *dst = cpu_info.brand;
            while (*p) {
                *dst++ = *p++;
            }
            *dst = '\0';
        }
    } else {
        str_copy(cpu_info.brand, "Unknown", 8);
    }

    /* Try to get TSC frequency */
    cpuid_call(0, &eax, &ebx, &ecx, &edx);

    if (eax >= 0x15) {
        /* CPUID leaf 15h provides TSC/Core Crystal Clock ratio */
        cpuid_call(0x15, &eax, &ebx, &ecx, &edx);

        if (eax && ebx && ecx) {
            /* TSC frequency = (Crystal Clock * EBX) / EAX */
            cpu_info.tsc_freq = ((uint64_t)ecx * ebx) / eax;
        }
    }

    if (cpu_info.tsc_freq == 0) {
        /* Default estimate for QEMU */
        cpu_info.tsc_freq = 1000000000UL;  /* 1 GHz */
    }
}
