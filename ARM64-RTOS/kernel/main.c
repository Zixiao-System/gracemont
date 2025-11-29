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
 * ARM64 RTOS - Main Entry Point
 */

#include "rtos_types.h"
#include "rtos_config.h"
#include "zbuf.h"
#include "net_stack.h"
#include "modbus.h"
#include "opcua.h"
#include "profinet.h"

/* External Functions */
extern void heap_init(void);
extern void dma_pool_init(void);
extern void gic_init(void);
extern void scheduler_start(void);
extern status_t task_create(tcb_t *tcb, const char *name, void (*entry)(void *),
                            void *arg, uint8_t priority, void *stack, size_t stack_size);
extern status_t task_start(tcb_t *tcb);
extern void irq_enable(uint32_t irq);
extern status_t irq_register(uint32_t irq, irq_handler_t handler, void *arg);

/* UART Driver (简化实现) */
#define UART_DR         (*(volatile uint32_t *)(CONFIG_UART_BASE + 0x00))
#define UART_FR         (*(volatile uint32_t *)(CONFIG_UART_BASE + 0x18))

static void uart_putc(char c)
{
    while (UART_FR & (1 << 5));  /* Wait for TX FIFO not full */
    UART_DR = c;
}

static void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

static void print_hex(uint32_t val)
{
    char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uart_putc(hex[(val >> i) & 0xF]);
    }
}

/* Timer Driver */
static void timer_irq_handler(uint32_t irq, void *arg)
{
    extern void scheduler_tick(void);
    extern void timer_tick_handler(void);

    /* Acknowledge timer */
    uint64_t cval;
    __asm__ volatile("mrs %0, cntv_cval_el0" : "=r"(cval));
    cval += CONFIG_CPU_FREQ_HZ / CONFIG_TICK_RATE_HZ;
    __asm__ volatile("msr cntv_cval_el0, %0" : : "r"(cval));

    scheduler_tick();
    timer_tick_handler();
}

static void timer_init(void)
{
    /* Set timer compare value */
    uint64_t cnt;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(cnt));
    cnt += CONFIG_CPU_FREQ_HZ / CONFIG_TICK_RATE_HZ;
    __asm__ volatile("msr cntv_cval_el0, %0" : : "r"(cnt));

    /* Enable timer */
    __asm__ volatile("msr cntv_ctl_el0, %0" : : "r"(1));

    /* Register and enable IRQ */
    irq_register(CONFIG_TIMER_IRQ, timer_irq_handler, NULL);
    irq_enable(CONFIG_TIMER_IRQ);
}

/* Network Interface (简化以太网驱动) */
static netif_t eth0;

static status_t eth_send(netif_t *nif, zbuf_t *zb)
{
    /* 实际实现会将数据写入DMA描述符 */
    /* 这里简化处理 */
    zbuf_free(zb);
    return STATUS_OK;
}

static void eth_init(void)
{
    /* Initialize network interface */
    eth0.name[0] = 'e'; eth0.name[1] = 't'; eth0.name[2] = 'h';
    eth0.name[3] = '0'; eth0.name[4] = '\0';

    /* MAC address */
    eth0.mac[0] = 0x02;
    eth0.mac[1] = 0x00;
    eth0.mac[2] = 0x00;
    eth0.mac[3] = 0x00;
    eth0.mac[4] = 0x00;
    eth0.mac[5] = 0x01;

    /* IP configuration */
    eth0.ip = IP4_ADDR(192, 168, 1, 100);
    eth0.netmask = IP4_ADDR(255, 255, 255, 0);
    eth0.gateway = IP4_ADDR(192, 168, 1, 1);
    eth0.mtu = 1500;
    eth0.up = true;

    eth0.send = eth_send;
    eth0.priv = NULL;

    netif_register(&eth0);
}

/* Protocol Contexts */
static modbus_server_t modbus_server;
static modbus_data_t modbus_data;
static uint8_t modbus_coils[256];
static uint16_t modbus_holding_regs[256];
static uint16_t modbus_input_regs[256];

static opcua_server_t opcua_server;
static pnio_device_t profinet_device;

/* Task Stacks */
static uint8_t main_task_stack[CONFIG_TASK_STACK_SIZE] ALIGNED(16);
static uint8_t modbus_task_stack[CONFIG_TASK_STACK_SIZE] ALIGNED(16);
static uint8_t opcua_task_stack[CONFIG_TASK_STACK_SIZE] ALIGNED(16);
static uint8_t profinet_task_stack[CONFIG_TASK_STACK_SIZE] ALIGNED(16);

static tcb_t main_tcb;
static tcb_t modbus_tcb;
static tcb_t opcua_tcb;
static tcb_t profinet_tcb;

/*
 * Modbus Server Task
 */
static void modbus_task(void *arg)
{
    uart_puts("[Modbus] Starting server on port 502\n");

    /* Initialize data model */
    modbus_data.coils = modbus_coils;
    modbus_data.coils_count = CONFIG_MODBUS_MAX_COILS;
    modbus_data.discrete_inputs = modbus_coils;
    modbus_data.discrete_inputs_count = CONFIG_MODBUS_MAX_COILS;
    modbus_data.holding_registers = modbus_holding_regs;
    modbus_data.holding_registers_count = CONFIG_MODBUS_MAX_REGS;
    modbus_data.input_registers = modbus_input_regs;
    modbus_data.input_registers_count = CONFIG_MODBUS_MAX_REGS;
    modbus_data.lock = (spinlock_t)SPINLOCK_INIT;

    modbus_server_init(&modbus_server, CONFIG_MODBUS_SLAVE_ADDR, &modbus_data);
    modbus_tcp_server_start(&modbus_server, CONFIG_MODBUS_TCP_PORT);

    while (1) {
        modbus_server_poll(&modbus_server);
        task_sleep(1);
    }
}

/*
 * OPC UA Server Task
 */
static void opcua_task(void *arg)
{
    uart_puts("[OPC UA] Starting server on port 4840\n");

    opcua_server_init(&opcua_server);

    /* Create address space */
    opcua_nodeid_t root_id = { .ns = 0, .type = OPCUA_NODEID_NUMERIC, .id.numeric = 84 };
    opcua_node_t *root = opcua_add_node(&opcua_server, NULL, &root_id,
                                         OPCUA_NC_OBJECT, "Root", "Root Folder");

    opcua_nodeid_t objects_id = { .ns = 0, .type = OPCUA_NODEID_NUMERIC, .id.numeric = 85 };
    opcua_node_t *objects = opcua_add_node(&opcua_server, root, &objects_id,
                                            OPCUA_NC_OBJECT, "Objects", "Objects");

    /* Add process variables */
    opcua_nodeid_t temp_id = { .ns = 1, .type = OPCUA_NODEID_NUMERIC, .id.numeric = 1001 };
    opcua_node_t *temp_node = opcua_add_node(&opcua_server, objects, &temp_id,
                                              OPCUA_NC_VARIABLE, "Temperature", "Temperature");
    if (temp_node) {
        opcua_variant_t val = { .type = OPCUA_TYPE_FLOAT, .value.f32 = 25.5f };
        opcua_set_value(temp_node, &val);
    }

    opcua_nodeid_t pressure_id = { .ns = 1, .type = OPCUA_NODEID_NUMERIC, .id.numeric = 1002 };
    opcua_node_t *pressure_node = opcua_add_node(&opcua_server, objects, &pressure_id,
                                                  OPCUA_NC_VARIABLE, "Pressure", "Pressure");
    if (pressure_node) {
        opcua_variant_t val = { .type = OPCUA_TYPE_FLOAT, .value.f32 = 101.3f };
        opcua_set_value(pressure_node, &val);
    }

    opcua_server_start(&opcua_server, CONFIG_OPCUA_PORT);

    while (1) {
        opcua_server_poll(&opcua_server);
        task_sleep(10);
    }
}

/*
 * PROFINET RT Task
 */
static void profinet_task(void *arg)
{
    uart_puts("[PROFINET] Starting device\n");

    pnio_device_init(&profinet_device, &eth0, "rtos-device", 0x1234, 0x5678);

    /* Add slots and subslots */
    pnio_add_slot(&profinet_device, 0, 0x00000001);  /* DAP */
    pnio_add_subslot(&profinet_device, 0, 1, 0x00000001, 0, 0);

    pnio_add_slot(&profinet_device, 1, 0x00000010);  /* IO Module */
    pnio_add_subslot(&profinet_device, 1, 1, 0x00000001, 8, 8);  /* 8 bytes I/O */

    pnio_plug_submodule(&profinet_device, 0, 1);
    pnio_plug_submodule(&profinet_device, 1, 1);

    pnio_device_start(&profinet_device);

    while (1) {
        pnio_device_poll(&profinet_device);

        /* Update I/O data */
        uint8_t *input = pnio_get_input_data(&profinet_device, 1, 1);
        if (input) {
            /* 模拟数据更新 */
            static uint8_t counter = 0;
            input[0] = counter++;
            input[1] = (uint8_t)(get_system_ticks() & 0xFF);
        }

        task_sleep(1);  /* 1ms cycle */
    }
}

/*
 * Main Application Task
 */
static void main_task(void *arg)
{
    uart_puts("\n");
    uart_puts("========================================\n");
    uart_puts(" Gracemont Industrial Control Framework\n");
    uart_puts(" ARM64 RTOS with Zero-Copy Protocols\n");
    uart_puts("========================================\n");
    uart_puts("\n");

    /* Print system info */
    uart_puts("System Configuration:\n");
    uart_puts("  CPU: ARM64 @ ");
    print_hex(CONFIG_CPU_FREQ_HZ);
    uart_puts(" Hz\n");
    uart_puts("  Heap: ");
    print_hex(CONFIG_HEAP_SIZE);
    uart_puts(" bytes\n");
    uart_puts("  Zero-Copy Buffers: ");
    print_hex(CONFIG_ZBUF_COUNT);
    uart_puts(" x ");
    print_hex(CONFIG_ZBUF_SIZE);
    uart_puts(" bytes\n");
    uart_puts("\n");

    /* Print network info */
    uart_puts("Network Configuration:\n");
    uart_puts("  Interface: eth0\n");
    uart_puts("  IP: 192.168.1.100\n");
    uart_puts("\n");

    /* Print protocol info */
    uart_puts("Industrial Protocols:\n");
    uart_puts("  Modbus TCP: Port 502\n");
    uart_puts("  OPC UA: Port 4840\n");
    uart_puts("  PROFINET RT: Active\n");
    uart_puts("\n");

    uart_puts("System running...\n\n");

    /* Main loop - system monitoring */
    uint32_t uptime = 0;
    while (1) {
        task_sleep(1000);  /* 1 second */
        uptime++;

        if ((uptime % 60) == 0) {
            uart_puts("[System] Uptime: ");
            print_hex(uptime);
            uart_puts(" seconds\n");

            /* Print buffer stats */
            uint32_t total, free, failures;
            zbuf_pool_stats(&total, &free, &failures);
            uart_puts("  ZBuf Free: ");
            print_hex(free);
            uart_puts("/");
            print_hex(total);
            uart_puts("\n");
        }
    }
}

/*
 * Exception Handlers
 */
void sync_exception_handler(void *frame)
{
    uint64_t esr, elr, far;
    __asm__ volatile("mrs %0, esr_el1" : "=r"(esr));
    __asm__ volatile("mrs %0, elr_el1" : "=r"(elr));
    __asm__ volatile("mrs %0, far_el1" : "=r"(far));

    uart_puts("\n!!! SYNC EXCEPTION !!!\n");
    uart_puts("ESR: ");
    print_hex(esr);
    uart_puts("\n");
    uart_puts("ELR: ");
    print_hex(elr);
    uart_puts("\n");
    uart_puts("FAR: ");
    print_hex(far);
    uart_puts("\n");

    while (1) {
        __asm__ volatile("wfi");
    }
}

void fiq_handler(void)
{
    uart_puts("FIQ!\n");
}

void serror_handler(void *frame)
{
    uart_puts("SError!\n");
    while (1) {
        __asm__ volatile("wfi");
    }
}

/*
 * Kernel Main Entry
 */
void kernel_main(void)
{
    /* Initialize subsystems */
    uart_puts("Initializing memory...\n");
    heap_init();
    dma_pool_init();

    uart_puts("Initializing zero-copy buffers...\n");
    zbuf_pool_init();

    uart_puts("Initializing interrupt controller...\n");
    gic_init();

    uart_puts("Initializing timer...\n");
    timer_init();

    uart_puts("Initializing network stack...\n");
    net_stack_init();
    eth_init();

    uart_puts("Creating tasks...\n");

    /* Create application tasks */
    task_create(&main_tcb, "main", main_task, NULL, 8,
                main_task_stack, sizeof(main_task_stack));
    task_create(&modbus_tcb, "modbus", modbus_task, NULL, 10,
                modbus_task_stack, sizeof(modbus_task_stack));
    task_create(&opcua_tcb, "opcua", opcua_task, NULL, 10,
                opcua_task_stack, sizeof(opcua_task_stack));
    task_create(&profinet_tcb, "profinet", profinet_task, NULL, 12,
                profinet_task_stack, sizeof(profinet_task_stack));

    /* Start tasks */
    task_start(&main_tcb);
    task_start(&modbus_tcb);
    task_start(&opcua_tcb);
    task_start(&profinet_tcb);

    uart_puts("Starting scheduler...\n\n");

    /* Start scheduler - never returns */
    scheduler_start();
}
