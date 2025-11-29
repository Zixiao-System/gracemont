/*
 * Gracemont Industrial Control Framework
 * ARM64 RTOS Configuration
 * Zero-Copy Industrial Protocol Stack
 */

#ifndef RTOS_CONFIG_H
#define RTOS_CONFIG_H

/* System Configuration */
#define CONFIG_CPU_FREQ_HZ           1000000000UL  /* 1 GHz */
#define CONFIG_TICK_RATE_HZ          1000          /* 1ms tick */
#define CONFIG_MAX_TASKS             32
#define CONFIG_MAX_PRIORITY          16
#define CONFIG_TASK_STACK_SIZE       4096
#define CONFIG_IDLE_STACK_SIZE       1024

/* Memory Configuration */
#define CONFIG_HEAP_SIZE             (16 * 1024 * 1024)  /* 16 MB */
#define CONFIG_DMA_POOL_SIZE         (4 * 1024 * 1024)   /* 4 MB for zero-copy */
#define CONFIG_ZBUF_POOL_SIZE        (2 * 1024 * 1024)   /* 2 MB zero-copy buffers */

/* Zero-Copy Buffer Configuration */
#define CONFIG_ZBUF_COUNT            1024
#define CONFIG_ZBUF_SIZE             2048          /* MTU + headers */
#define CONFIG_ZBUF_HEADROOM         128           /* Space for protocol headers */

/* Network Configuration */
#define CONFIG_NET_RX_RING_SIZE      256
#define CONFIG_NET_TX_RING_SIZE      256
#define CONFIG_NET_MAX_SOCKETS       64
#define CONFIG_TCP_MSS               1460
#define CONFIG_TCP_WINDOW_SIZE       65535

/* Modbus Configuration */
#define CONFIG_MODBUS_TCP_PORT       502
#define CONFIG_MODBUS_RTU_BAUD       115200
#define CONFIG_MODBUS_MAX_REGS       256
#define CONFIG_MODBUS_MAX_COILS      2048
#define CONFIG_MODBUS_SLAVE_ADDR     1

/* OPC UA Configuration */
#define CONFIG_OPCUA_PORT            4840
#define CONFIG_OPCUA_MAX_SESSIONS    8
#define CONFIG_OPCUA_MAX_SUBSCRIPTIONS 16
#define CONFIG_OPCUA_MAX_NODES       1024
#define CONFIG_OPCUA_SECURITY_NONE   1
#define CONFIG_OPCUA_SECURITY_SIGN   0
#define CONFIG_OPCUA_SECURITY_ENCRYPT 0

/* PROFINET Configuration */
#define CONFIG_PROFINET_CYCLE_TIME   1000          /* 1ms cycle */
#define CONFIG_PROFINET_MAX_DEVICES  32
#define CONFIG_PROFINET_MAX_SLOTS    16
#define CONFIG_PROFINET_MAX_SUBSLOTS 8
#define CONFIG_PROFINET_RT_CLASS     1             /* RT Class 1 */

/* ARM64 GIC Configuration */
#define CONFIG_GICD_BASE             0x08000000
#define CONFIG_GICC_BASE             0x08010000
#define CONFIG_GIC_SPI_START         32

/* Timer Configuration */
#define CONFIG_TIMER_IRQ             27            /* Generic Timer PPI */

/* UART Configuration */
#define CONFIG_UART_BASE             0x09000000
#define CONFIG_UART_IRQ              33

/* Ethernet Configuration */
#define CONFIG_ETH_BASE              0x0A000000
#define CONFIG_ETH_IRQ               47

/* Debug Configuration */
#define CONFIG_DEBUG_UART            1
#define CONFIG_DEBUG_LEVEL           2             /* 0=none, 1=error, 2=warn, 3=info */

#endif /* RTOS_CONFIG_H */
