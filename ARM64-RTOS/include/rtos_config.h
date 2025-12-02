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
 * ARM64 RTOS Configuration
 * Zero-Copy Industrial Protocol Stack
 *
 * Configuration is managed via Kconfig (make menuconfig).
 * This file includes the auto-generated configuration and provides
 * fallback defaults if autoconf.h is not present.
 */

#ifndef RTOS_CONFIG_H
#define RTOS_CONFIG_H

/* Include auto-generated configuration if available */
#if __has_include("autoconf.h")
#include "autoconf.h"
#else
/* Fallback defaults when autoconf.h is not generated */
#warning "autoconf.h not found, using default configuration. Run 'make defconfig' to generate."

/* System Configuration */
#ifndef CONFIG_CPU_FREQ_HZ
#define CONFIG_CPU_FREQ_HZ           1000000000UL  /* 1 GHz */
#endif
#ifndef CONFIG_TICK_RATE_HZ
#define CONFIG_TICK_RATE_HZ          1000          /* 1ms tick */
#endif
#ifndef CONFIG_MAX_TASKS
#define CONFIG_MAX_TASKS             32
#endif
#ifndef CONFIG_MAX_PRIORITY
#define CONFIG_MAX_PRIORITY          16
#endif
#ifndef CONFIG_TASK_STACK_SIZE
#define CONFIG_TASK_STACK_SIZE       4096
#endif
#ifndef CONFIG_IDLE_STACK_SIZE
#define CONFIG_IDLE_STACK_SIZE       1024
#endif

/* Memory Configuration */
#ifndef CONFIG_HEAP_SIZE
#define CONFIG_HEAP_SIZE             (16 * 1024 * 1024)  /* 16 MB */
#endif
#ifndef CONFIG_DMA_POOL_SIZE
#define CONFIG_DMA_POOL_SIZE         (4 * 1024 * 1024)   /* 4 MB for zero-copy */
#endif
#ifndef CONFIG_ZBUF_POOL_SIZE
#define CONFIG_ZBUF_POOL_SIZE        (2 * 1024 * 1024)   /* 2 MB zero-copy buffers */
#endif

/* Zero-Copy Buffer Configuration */
#ifndef CONFIG_ZBUF_COUNT
#define CONFIG_ZBUF_COUNT            1024
#endif
#ifndef CONFIG_ZBUF_SIZE
#define CONFIG_ZBUF_SIZE             2048          /* MTU + headers */
#endif
#ifndef CONFIG_ZBUF_HEADROOM
#define CONFIG_ZBUF_HEADROOM         128           /* Space for protocol headers */
#endif

/* Kernel Configuration */
#ifndef CONFIG_KERNEL_PREEMPTION
#define CONFIG_KERNEL_PREEMPTION     1
#endif
#ifndef CONFIG_KERNEL_TIMESLICE
#define CONFIG_KERNEL_TIMESLICE      10
#endif
#ifndef CONFIG_KERNEL_STACK_CHECK
#define CONFIG_KERNEL_STACK_CHECK    1
#endif
#ifndef CONFIG_KERNEL_IDLE_SLEEP
#define CONFIG_KERNEL_IDLE_SLEEP     1
#endif

/* Network Configuration */
#ifndef CONFIG_NET_ENABLED
#define CONFIG_NET_ENABLED           1
#endif
#ifndef CONFIG_NET_RX_RING_SIZE
#define CONFIG_NET_RX_RING_SIZE      256
#endif
#ifndef CONFIG_NET_TX_RING_SIZE
#define CONFIG_NET_TX_RING_SIZE      256
#endif
#ifndef CONFIG_NET_MAX_SOCKETS
#define CONFIG_NET_MAX_SOCKETS       64
#endif
#ifndef CONFIG_TCP_MSS
#define CONFIG_TCP_MSS               1460
#endif
#ifndef CONFIG_TCP_WINDOW_SIZE
#define CONFIG_TCP_WINDOW_SIZE       65535
#endif

/* Modbus Configuration */
#ifndef CONFIG_MODBUS_ENABLED
#define CONFIG_MODBUS_ENABLED        1
#endif
#ifndef CONFIG_MODBUS_TCP_PORT
#define CONFIG_MODBUS_TCP_PORT       502
#endif
#ifndef CONFIG_MODBUS_RTU_BAUD
#define CONFIG_MODBUS_RTU_BAUD       115200
#endif
#ifndef CONFIG_MODBUS_MAX_REGS
#define CONFIG_MODBUS_MAX_REGS       256
#endif
#ifndef CONFIG_MODBUS_MAX_COILS
#define CONFIG_MODBUS_MAX_COILS      2048
#endif
#ifndef CONFIG_MODBUS_SLAVE_ADDR
#define CONFIG_MODBUS_SLAVE_ADDR     1
#endif

/* OPC UA Configuration */
#ifndef CONFIG_OPCUA_ENABLED
#define CONFIG_OPCUA_ENABLED         1
#endif
#ifndef CONFIG_OPCUA_PORT
#define CONFIG_OPCUA_PORT            4840
#endif
#ifndef CONFIG_OPCUA_MAX_SESSIONS
#define CONFIG_OPCUA_MAX_SESSIONS    8
#endif
#ifndef CONFIG_OPCUA_MAX_SUBSCRIPTIONS
#define CONFIG_OPCUA_MAX_SUBSCRIPTIONS 16
#endif
#ifndef CONFIG_OPCUA_MAX_NODES
#define CONFIG_OPCUA_MAX_NODES       1024
#endif
#ifndef CONFIG_OPCUA_SECURITY_NONE
#define CONFIG_OPCUA_SECURITY_NONE   1
#endif
#ifndef CONFIG_OPCUA_SECURITY_SIGN
#define CONFIG_OPCUA_SECURITY_SIGN   0
#endif
#ifndef CONFIG_OPCUA_SECURITY_ENCRYPT
#define CONFIG_OPCUA_SECURITY_ENCRYPT 0
#endif

/* PROFINET Configuration */
#ifndef CONFIG_PROFINET_ENABLED
#define CONFIG_PROFINET_ENABLED      1
#endif
#ifndef CONFIG_PROFINET_CYCLE_TIME
#define CONFIG_PROFINET_CYCLE_TIME   1000          /* 1ms cycle */
#endif
#ifndef CONFIG_PROFINET_MAX_DEVICES
#define CONFIG_PROFINET_MAX_DEVICES  32
#endif
#ifndef CONFIG_PROFINET_MAX_SLOTS
#define CONFIG_PROFINET_MAX_SLOTS    16
#endif
#ifndef CONFIG_PROFINET_MAX_SUBSLOTS
#define CONFIG_PROFINET_MAX_SUBSLOTS 8
#endif
#ifndef CONFIG_PROFINET_RT_CLASS
#define CONFIG_PROFINET_RT_CLASS     1             /* RT Class 1 */
#endif

/* ARM64 GIC Configuration */
#ifndef CONFIG_GICD_BASE
#define CONFIG_GICD_BASE             0x08000000
#endif
#ifndef CONFIG_GICC_BASE
#define CONFIG_GICC_BASE             0x08010000
#endif
#ifndef CONFIG_GIC_SPI_START
#define CONFIG_GIC_SPI_START         32
#endif

/* Timer Configuration */
#ifndef CONFIG_TIMER_IRQ
#define CONFIG_TIMER_IRQ             27            /* Generic Timer PPI */
#endif

/* UART Configuration */
#ifndef CONFIG_UART_ENABLED
#define CONFIG_UART_ENABLED          1
#endif
#ifndef CONFIG_UART_BASE
#define CONFIG_UART_BASE             0x09000000
#endif
#ifndef CONFIG_UART_IRQ
#define CONFIG_UART_IRQ              33
#endif
#ifndef CONFIG_UART_COUNT
#define CONFIG_UART_COUNT            1
#endif
#ifndef CONFIG_UART_CLOCK
#define CONFIG_UART_CLOCK            24000000    /* 24 MHz */
#endif

/* Ethernet Configuration */
#ifndef CONFIG_ETH_ENABLED
#define CONFIG_ETH_ENABLED           1
#endif
#ifndef CONFIG_ETH_BASE
#define CONFIG_ETH_BASE              0x0A000000
#endif
#ifndef CONFIG_ETH_IRQ
#define CONFIG_ETH_IRQ               47
#endif

/* Debug Configuration */
#ifndef CONFIG_DEBUG_UART
#define CONFIG_DEBUG_UART            1
#endif
#ifndef CONFIG_DEBUG_LEVEL
#define CONFIG_DEBUG_LEVEL           2             /* 0=none, 1=error, 2=warn, 3=info */
#endif

#endif /* __has_include("autoconf.h") */

#endif /* RTOS_CONFIG_H */
