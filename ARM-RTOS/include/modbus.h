/*
 * Gracemont Industrial Control Framework
 * Zero-Copy Modbus TCP/RTU Protocol Implementation
 */

#ifndef MODBUS_H
#define MODBUS_H

#include "rtos_types.h"
#include "zbuf.h"
#include "net_stack.h"

/* Modbus Function Codes */
#define MODBUS_FC_READ_COILS                0x01
#define MODBUS_FC_READ_DISCRETE_INPUTS      0x02
#define MODBUS_FC_READ_HOLDING_REGISTERS    0x03
#define MODBUS_FC_READ_INPUT_REGISTERS      0x04
#define MODBUS_FC_WRITE_SINGLE_COIL         0x05
#define MODBUS_FC_WRITE_SINGLE_REGISTER     0x06
#define MODBUS_FC_WRITE_MULTIPLE_COILS      0x0F
#define MODBUS_FC_WRITE_MULTIPLE_REGISTERS  0x10
#define MODBUS_FC_READ_WRITE_REGISTERS      0x17

/* Modbus Exception Codes */
#define MODBUS_EX_ILLEGAL_FUNCTION          0x01
#define MODBUS_EX_ILLEGAL_DATA_ADDRESS      0x02
#define MODBUS_EX_ILLEGAL_DATA_VALUE        0x03
#define MODBUS_EX_SLAVE_DEVICE_FAILURE      0x04
#define MODBUS_EX_ACKNOWLEDGE               0x05
#define MODBUS_EX_SLAVE_BUSY                0x06
#define MODBUS_EX_MEMORY_PARITY_ERROR       0x08
#define MODBUS_EX_GATEWAY_PATH_UNAVAILABLE  0x0A
#define MODBUS_EX_GATEWAY_TARGET_FAILED     0x0B

/* Modbus TCP Header (MBAP) */
typedef struct PACKED {
    uint16_t    transaction_id;
    uint16_t    protocol_id;
    uint16_t    length;
    uint8_t     unit_id;
} modbus_tcp_hdr_t;

#define MODBUS_TCP_HDR_LEN  7

/* Modbus PDU */
typedef struct PACKED {
    uint8_t     function;
    uint8_t     data[];
} modbus_pdu_t;

/* Modbus RTU Frame */
typedef struct PACKED {
    uint8_t     address;
    uint8_t     function;
    uint8_t     data[];
    /* CRC16 at end */
} modbus_rtu_frame_t;

/* Data Model */
typedef struct {
    /* Coils (R/W bits) */
    uint8_t     *coils;
    uint16_t    coils_count;

    /* Discrete Inputs (RO bits) */
    uint8_t     *discrete_inputs;
    uint16_t    discrete_inputs_count;

    /* Holding Registers (R/W 16-bit) */
    uint16_t    *holding_registers;
    uint16_t    holding_registers_count;

    /* Input Registers (RO 16-bit) */
    uint16_t    *input_registers;
    uint16_t    input_registers_count;

    /* Callbacks for custom handling */
    status_t    (*on_read_coils)(uint16_t addr, uint16_t count);
    status_t    (*on_write_coils)(uint16_t addr, uint16_t count);
    status_t    (*on_read_holding)(uint16_t addr, uint16_t count);
    status_t    (*on_write_holding)(uint16_t addr, uint16_t count);

    spinlock_t  lock;
} modbus_data_t;

/* Modbus Server Context */
typedef struct {
    uint8_t         slave_addr;
    modbus_data_t   *data;

    /* TCP Server */
    int             tcp_socket;
    bool            tcp_running;

    /* RTU Interface */
    void            *uart_handle;
    bool            rtu_running;

    /* Statistics */
    uint32_t        requests;
    uint32_t        responses;
    uint32_t        errors;
    uint32_t        exceptions;
} modbus_server_t;

/* Modbus Client Context */
typedef struct {
    uint8_t         slave_addr;
    uint16_t        transaction_id;

    /* TCP Client */
    int             tcp_socket;
    sockaddr_t      server_addr;

    /* RTU Interface */
    void            *uart_handle;

    /* Timeout */
    tick_t          timeout;
} modbus_client_t;

/* Server API */
status_t modbus_server_init(modbus_server_t *server, uint8_t slave_addr, modbus_data_t *data);
status_t modbus_tcp_server_start(modbus_server_t *server, uint16_t port);
status_t modbus_rtu_server_start(modbus_server_t *server, void *uart);
void modbus_server_stop(modbus_server_t *server);
void modbus_server_poll(modbus_server_t *server);

/* Client API */
status_t modbus_client_init(modbus_client_t *client);
status_t modbus_tcp_connect(modbus_client_t *client, uint32_t ip, uint16_t port);
status_t modbus_rtu_init(modbus_client_t *client, void *uart);
void modbus_client_close(modbus_client_t *client);

/* Client Read Functions (Zero-Copy) */
status_t modbus_read_coils(modbus_client_t *client, uint8_t slave,
                           uint16_t addr, uint16_t count, uint8_t *result);
status_t modbus_read_discrete_inputs(modbus_client_t *client, uint8_t slave,
                                      uint16_t addr, uint16_t count, uint8_t *result);
status_t modbus_read_holding_registers(modbus_client_t *client, uint8_t slave,
                                        uint16_t addr, uint16_t count, uint16_t *result);
status_t modbus_read_input_registers(modbus_client_t *client, uint8_t slave,
                                      uint16_t addr, uint16_t count, uint16_t *result);

/* Client Write Functions (Zero-Copy) */
status_t modbus_write_single_coil(modbus_client_t *client, uint8_t slave,
                                   uint16_t addr, bool value);
status_t modbus_write_single_register(modbus_client_t *client, uint8_t slave,
                                       uint16_t addr, uint16_t value);
status_t modbus_write_multiple_coils(modbus_client_t *client, uint8_t slave,
                                      uint16_t addr, uint16_t count, const uint8_t *values);
status_t modbus_write_multiple_registers(modbus_client_t *client, uint8_t slave,
                                          uint16_t addr, uint16_t count, const uint16_t *values);

/* Zero-Copy Raw API */
zbuf_t *modbus_build_request(modbus_client_t *client, uint8_t slave,
                              uint8_t function, const void *data, uint16_t len);
status_t modbus_send_request(modbus_client_t *client, zbuf_t *zb);
zbuf_t *modbus_recv_response(modbus_client_t *client);

/* CRC Calculation */
uint16_t modbus_crc16(const uint8_t *data, size_t len);

#endif /* MODBUS_H */
