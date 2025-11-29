/*
 * Gracemont Industrial Control Framework
 * Zero-Copy Modbus TCP/RTU Implementation
 */

#include "modbus.h"
#include "rtos_config.h"

/* CRC16 Table for Modbus RTU */
static const uint16_t crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

uint16_t modbus_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc = (crc >> 8) ^ crc16_table[(crc ^ *data++) & 0xFF];
    }
    return crc;
}

/*
 * Server Initialization
 */
status_t modbus_server_init(modbus_server_t *server, uint8_t slave_addr, modbus_data_t *data)
{
    server->slave_addr = slave_addr;
    server->data = data;
    server->tcp_socket = -1;
    server->tcp_running = false;
    server->uart_handle = NULL;
    server->rtu_running = false;
    server->requests = 0;
    server->responses = 0;
    server->errors = 0;
    server->exceptions = 0;

    return STATUS_OK;
}

/*
 * Build Exception Response (Zero-Copy)
 */
static zbuf_t *modbus_build_exception(uint8_t function, uint8_t exception)
{
    zbuf_t *zb = zbuf_alloc_tx(2);
    if (zb == NULL) return NULL;

    uint8_t *data = zbuf_put(zb, 2);
    data[0] = function | 0x80;
    data[1] = exception;

    return zb;
}

/*
 * Process Modbus Request (Zero-Copy)
 */
static zbuf_t *modbus_process_request(modbus_server_t *server, zbuf_t *req)
{
    modbus_data_t *data = server->data;
    uint8_t *pdu = req->data;
    uint8_t function = pdu[0];
    zbuf_t *resp = NULL;

    server->requests++;

    spin_lock(&data->lock);

    switch (function) {
    case MODBUS_FC_READ_COILS: {
        uint16_t addr = (pdu[1] << 8) | pdu[2];
        uint16_t count = (pdu[3] << 8) | pdu[4];

        if (addr + count > data->coils_count) {
            resp = modbus_build_exception(function, MODBUS_EX_ILLEGAL_DATA_ADDRESS);
            server->exceptions++;
            break;
        }

        if (data->on_read_coils) {
            data->on_read_coils(addr, count);
        }

        uint8_t byte_count = (count + 7) / 8;
        resp = zbuf_alloc_tx(2 + byte_count);
        if (resp == NULL) break;

        uint8_t *out = zbuf_put(resp, 2 + byte_count);
        out[0] = function;
        out[1] = byte_count;

        for (uint8_t i = 0; i < byte_count; i++) {
            out[2 + i] = 0;
            for (int bit = 0; bit < 8 && (i * 8 + bit) < count; bit++) {
                uint16_t coil_addr = addr + i * 8 + bit;
                if (data->coils[coil_addr / 8] & (1 << (coil_addr % 8))) {
                    out[2 + i] |= (1 << bit);
                }
            }
        }
        break;
    }

    case MODBUS_FC_READ_DISCRETE_INPUTS: {
        uint16_t addr = (pdu[1] << 8) | pdu[2];
        uint16_t count = (pdu[3] << 8) | pdu[4];

        if (addr + count > data->discrete_inputs_count) {
            resp = modbus_build_exception(function, MODBUS_EX_ILLEGAL_DATA_ADDRESS);
            server->exceptions++;
            break;
        }

        uint8_t byte_count = (count + 7) / 8;
        resp = zbuf_alloc_tx(2 + byte_count);
        if (resp == NULL) break;

        uint8_t *out = zbuf_put(resp, 2 + byte_count);
        out[0] = function;
        out[1] = byte_count;

        for (uint8_t i = 0; i < byte_count; i++) {
            out[2 + i] = 0;
            for (int bit = 0; bit < 8 && (i * 8 + bit) < count; bit++) {
                uint16_t input_addr = addr + i * 8 + bit;
                if (data->discrete_inputs[input_addr / 8] & (1 << (input_addr % 8))) {
                    out[2 + i] |= (1 << bit);
                }
            }
        }
        break;
    }

    case MODBUS_FC_READ_HOLDING_REGISTERS: {
        uint16_t addr = (pdu[1] << 8) | pdu[2];
        uint16_t count = (pdu[3] << 8) | pdu[4];

        if (addr + count > data->holding_registers_count) {
            resp = modbus_build_exception(function, MODBUS_EX_ILLEGAL_DATA_ADDRESS);
            server->exceptions++;
            break;
        }

        if (data->on_read_holding) {
            data->on_read_holding(addr, count);
        }

        uint8_t byte_count = count * 2;
        resp = zbuf_alloc_tx(2 + byte_count);
        if (resp == NULL) break;

        uint8_t *out = zbuf_put(resp, 2 + byte_count);
        out[0] = function;
        out[1] = byte_count;

        for (uint16_t i = 0; i < count; i++) {
            uint16_t val = data->holding_registers[addr + i];
            out[2 + i * 2] = val >> 8;
            out[2 + i * 2 + 1] = val & 0xFF;
        }
        break;
    }

    case MODBUS_FC_READ_INPUT_REGISTERS: {
        uint16_t addr = (pdu[1] << 8) | pdu[2];
        uint16_t count = (pdu[3] << 8) | pdu[4];

        if (addr + count > data->input_registers_count) {
            resp = modbus_build_exception(function, MODBUS_EX_ILLEGAL_DATA_ADDRESS);
            server->exceptions++;
            break;
        }

        uint8_t byte_count = count * 2;
        resp = zbuf_alloc_tx(2 + byte_count);
        if (resp == NULL) break;

        uint8_t *out = zbuf_put(resp, 2 + byte_count);
        out[0] = function;
        out[1] = byte_count;

        for (uint16_t i = 0; i < count; i++) {
            uint16_t val = data->input_registers[addr + i];
            out[2 + i * 2] = val >> 8;
            out[2 + i * 2 + 1] = val & 0xFF;
        }
        break;
    }

    case MODBUS_FC_WRITE_SINGLE_COIL: {
        uint16_t addr = (pdu[1] << 8) | pdu[2];
        uint16_t value = (pdu[3] << 8) | pdu[4];

        if (addr >= data->coils_count) {
            resp = modbus_build_exception(function, MODBUS_EX_ILLEGAL_DATA_ADDRESS);
            server->exceptions++;
            break;
        }

        if (value == 0xFF00) {
            data->coils[addr / 8] |= (1 << (addr % 8));
        } else if (value == 0x0000) {
            data->coils[addr / 8] &= ~(1 << (addr % 8));
        } else {
            resp = modbus_build_exception(function, MODBUS_EX_ILLEGAL_DATA_VALUE);
            server->exceptions++;
            break;
        }

        if (data->on_write_coils) {
            data->on_write_coils(addr, 1);
        }

        /* Echo request */
        resp = zbuf_alloc_tx(5);
        if (resp == NULL) break;
        uint8_t *out = zbuf_put(resp, 5);
        for (int i = 0; i < 5; i++) out[i] = pdu[i];
        break;
    }

    case MODBUS_FC_WRITE_SINGLE_REGISTER: {
        uint16_t addr = (pdu[1] << 8) | pdu[2];
        uint16_t value = (pdu[3] << 8) | pdu[4];

        if (addr >= data->holding_registers_count) {
            resp = modbus_build_exception(function, MODBUS_EX_ILLEGAL_DATA_ADDRESS);
            server->exceptions++;
            break;
        }

        data->holding_registers[addr] = value;

        if (data->on_write_holding) {
            data->on_write_holding(addr, 1);
        }

        /* Echo request */
        resp = zbuf_alloc_tx(5);
        if (resp == NULL) break;
        uint8_t *out = zbuf_put(resp, 5);
        for (int i = 0; i < 5; i++) out[i] = pdu[i];
        break;
    }

    case MODBUS_FC_WRITE_MULTIPLE_COILS: {
        uint16_t addr = (pdu[1] << 8) | pdu[2];
        uint16_t count = (pdu[3] << 8) | pdu[4];
        uint8_t byte_count = pdu[5];

        if (addr + count > data->coils_count) {
            resp = modbus_build_exception(function, MODBUS_EX_ILLEGAL_DATA_ADDRESS);
            server->exceptions++;
            break;
        }

        for (uint16_t i = 0; i < count; i++) {
            uint16_t coil_addr = addr + i;
            if (pdu[6 + i / 8] & (1 << (i % 8))) {
                data->coils[coil_addr / 8] |= (1 << (coil_addr % 8));
            } else {
                data->coils[coil_addr / 8] &= ~(1 << (coil_addr % 8));
            }
        }

        if (data->on_write_coils) {
            data->on_write_coils(addr, count);
        }

        resp = zbuf_alloc_tx(5);
        if (resp == NULL) break;
        uint8_t *out = zbuf_put(resp, 5);
        out[0] = function;
        out[1] = pdu[1]; out[2] = pdu[2];
        out[3] = pdu[3]; out[4] = pdu[4];
        break;
    }

    case MODBUS_FC_WRITE_MULTIPLE_REGISTERS: {
        uint16_t addr = (pdu[1] << 8) | pdu[2];
        uint16_t count = (pdu[3] << 8) | pdu[4];
        uint8_t byte_count = pdu[5];

        if (addr + count > data->holding_registers_count) {
            resp = modbus_build_exception(function, MODBUS_EX_ILLEGAL_DATA_ADDRESS);
            server->exceptions++;
            break;
        }

        for (uint16_t i = 0; i < count; i++) {
            data->holding_registers[addr + i] =
                (pdu[6 + i * 2] << 8) | pdu[6 + i * 2 + 1];
        }

        if (data->on_write_holding) {
            data->on_write_holding(addr, count);
        }

        resp = zbuf_alloc_tx(5);
        if (resp == NULL) break;
        uint8_t *out = zbuf_put(resp, 5);
        out[0] = function;
        out[1] = pdu[1]; out[2] = pdu[2];
        out[3] = pdu[3]; out[4] = pdu[4];
        break;
    }

    default:
        resp = modbus_build_exception(function, MODBUS_EX_ILLEGAL_FUNCTION);
        server->exceptions++;
        break;
    }

    spin_unlock(&data->lock);

    if (resp != NULL) {
        server->responses++;
    } else {
        server->errors++;
    }

    return resp;
}

/*
 * TCP Server
 */
status_t modbus_tcp_server_start(modbus_server_t *server, uint16_t port)
{
    server->tcp_socket = sock_socket(SOCK_STREAM);
    if (server->tcp_socket < 0) {
        return STATUS_ERROR;
    }

    sockaddr_t addr = { .addr = IP4_ADDR_ANY, .port = port };
    if (sock_bind(server->tcp_socket, &addr) < 0) {
        sock_close(server->tcp_socket);
        return STATUS_ERROR;
    }

    if (sock_listen(server->tcp_socket, 5) < 0) {
        sock_close(server->tcp_socket);
        return STATUS_ERROR;
    }

    server->tcp_running = true;
    return STATUS_OK;
}

void modbus_server_poll(modbus_server_t *server)
{
    if (!server->tcp_running) return;

    /* Accept connection */
    sockaddr_t client_addr;
    int client_fd = sock_accept(server->tcp_socket, &client_addr);
    if (client_fd < 0) return;

    /* Receive request (zero-copy) */
    zbuf_t *zb = sock_recv_zbuf(client_fd);
    if (zb == NULL || zb->len < MODBUS_TCP_HDR_LEN + 1) {
        if (zb) zbuf_free(zb);
        sock_close(client_fd);
        return;
    }

    /* Parse MBAP header */
    modbus_tcp_hdr_t *mbap = (modbus_tcp_hdr_t *)zb->data;
    uint16_t trans_id = ntohs(mbap->transaction_id);
    uint16_t pdu_len = ntohs(mbap->length) - 1;

    /* Check unit ID */
    if (mbap->unit_id != server->slave_addr && mbap->unit_id != 0) {
        zbuf_free(zb);
        sock_close(client_fd);
        return;
    }

    /* Pull MBAP header */
    zbuf_pull(zb, MODBUS_TCP_HDR_LEN);

    /* Process request */
    zbuf_t *resp = modbus_process_request(server, zb);
    zbuf_free(zb);

    if (resp != NULL) {
        /* Push MBAP header */
        modbus_tcp_hdr_t *resp_mbap = (modbus_tcp_hdr_t *)zbuf_push(resp, MODBUS_TCP_HDR_LEN);
        resp_mbap->transaction_id = htons(trans_id);
        resp_mbap->protocol_id = 0;
        resp_mbap->length = htons(resp->len - MODBUS_TCP_HDR_LEN + 1);
        resp_mbap->unit_id = server->slave_addr;

        /* Send response (zero-copy) */
        sock_send_zbuf(client_fd, resp);
    }

    sock_close(client_fd);
}

/*
 * Client Functions
 */
status_t modbus_client_init(modbus_client_t *client)
{
    client->slave_addr = 1;
    client->transaction_id = 0;
    client->tcp_socket = -1;
    client->uart_handle = NULL;
    client->timeout = 1000;
    return STATUS_OK;
}

status_t modbus_tcp_connect(modbus_client_t *client, uint32_t ip, uint16_t port)
{
    client->tcp_socket = sock_socket(SOCK_STREAM);
    if (client->tcp_socket < 0) {
        return STATUS_ERROR;
    }

    client->server_addr.addr = ip;
    client->server_addr.port = port;

    if (sock_connect(client->tcp_socket, &client->server_addr) < 0) {
        sock_close(client->tcp_socket);
        client->tcp_socket = -1;
        return STATUS_ERROR;
    }

    return STATUS_OK;
}

void modbus_client_close(modbus_client_t *client)
{
    if (client->tcp_socket >= 0) {
        sock_close(client->tcp_socket);
        client->tcp_socket = -1;
    }
}

/*
 * Build Modbus TCP Request (Zero-Copy)
 */
zbuf_t *modbus_build_request(modbus_client_t *client, uint8_t slave,
                              uint8_t function, const void *data, uint16_t len)
{
    zbuf_t *zb = zbuf_alloc_tx(MODBUS_TCP_HDR_LEN + 1 + len);
    if (zb == NULL) return NULL;

    uint8_t *buf = zbuf_put(zb, MODBUS_TCP_HDR_LEN + 1 + len);

    /* MBAP Header */
    modbus_tcp_hdr_t *mbap = (modbus_tcp_hdr_t *)buf;
    mbap->transaction_id = htons(client->transaction_id++);
    mbap->protocol_id = 0;
    mbap->length = htons(len + 2);
    mbap->unit_id = slave;

    /* PDU */
    buf[MODBUS_TCP_HDR_LEN] = function;
    if (data != NULL && len > 0) {
        const uint8_t *src = (const uint8_t *)data;
        for (uint16_t i = 0; i < len; i++) {
            buf[MODBUS_TCP_HDR_LEN + 1 + i] = src[i];
        }
    }

    return zb;
}

status_t modbus_send_request(modbus_client_t *client, zbuf_t *zb)
{
    return (sock_send_zbuf(client->tcp_socket, zb) == 0) ? STATUS_OK : STATUS_ERROR;
}

zbuf_t *modbus_recv_response(modbus_client_t *client)
{
    return sock_recv_zbuf(client->tcp_socket);
}

/*
 * Read Holding Registers
 */
status_t modbus_read_holding_registers(modbus_client_t *client, uint8_t slave,
                                        uint16_t addr, uint16_t count, uint16_t *result)
{
    uint8_t req_data[4];
    req_data[0] = addr >> 8;
    req_data[1] = addr & 0xFF;
    req_data[2] = count >> 8;
    req_data[3] = count & 0xFF;

    zbuf_t *req = modbus_build_request(client, slave,
                                        MODBUS_FC_READ_HOLDING_REGISTERS,
                                        req_data, 4);
    if (req == NULL) return STATUS_NO_MEM;

    status_t ret = modbus_send_request(client, req);
    if (ret != STATUS_OK) return ret;

    zbuf_t *resp = modbus_recv_response(client);
    if (resp == NULL) return STATUS_TIMEOUT;

    /* Parse response */
    uint8_t *data = resp->data + MODBUS_TCP_HDR_LEN;
    if (data[0] & 0x80) {
        /* Exception */
        ret = STATUS_ERROR;
    } else {
        uint8_t byte_count = data[1];
        for (uint16_t i = 0; i < count; i++) {
            result[i] = (data[2 + i * 2] << 8) | data[2 + i * 2 + 1];
        }
        ret = STATUS_OK;
    }

    zbuf_free(resp);
    return ret;
}

/*
 * Write Single Register
 */
status_t modbus_write_single_register(modbus_client_t *client, uint8_t slave,
                                       uint16_t addr, uint16_t value)
{
    uint8_t req_data[4];
    req_data[0] = addr >> 8;
    req_data[1] = addr & 0xFF;
    req_data[2] = value >> 8;
    req_data[3] = value & 0xFF;

    zbuf_t *req = modbus_build_request(client, slave,
                                        MODBUS_FC_WRITE_SINGLE_REGISTER,
                                        req_data, 4);
    if (req == NULL) return STATUS_NO_MEM;

    status_t ret = modbus_send_request(client, req);
    if (ret != STATUS_OK) return ret;

    zbuf_t *resp = modbus_recv_response(client);
    if (resp == NULL) return STATUS_TIMEOUT;

    uint8_t *data = resp->data + MODBUS_TCP_HDR_LEN;
    ret = (data[0] & 0x80) ? STATUS_ERROR : STATUS_OK;

    zbuf_free(resp);
    return ret;
}

/*
 * Write Multiple Registers
 */
status_t modbus_write_multiple_registers(modbus_client_t *client, uint8_t slave,
                                          uint16_t addr, uint16_t count,
                                          const uint16_t *values)
{
    uint16_t data_len = 5 + count * 2;
    uint8_t *req_data = heap_alloc(data_len);
    if (req_data == NULL) return STATUS_NO_MEM;

    req_data[0] = addr >> 8;
    req_data[1] = addr & 0xFF;
    req_data[2] = count >> 8;
    req_data[3] = count & 0xFF;
    req_data[4] = count * 2;

    for (uint16_t i = 0; i < count; i++) {
        req_data[5 + i * 2] = values[i] >> 8;
        req_data[5 + i * 2 + 1] = values[i] & 0xFF;
    }

    zbuf_t *req = modbus_build_request(client, slave,
                                        MODBUS_FC_WRITE_MULTIPLE_REGISTERS,
                                        req_data, data_len);
    heap_free(req_data);

    if (req == NULL) return STATUS_NO_MEM;

    status_t ret = modbus_send_request(client, req);
    if (ret != STATUS_OK) return ret;

    zbuf_t *resp = modbus_recv_response(client);
    if (resp == NULL) return STATUS_TIMEOUT;

    uint8_t *data = resp->data + MODBUS_TCP_HDR_LEN;
    ret = (data[0] & 0x80) ? STATUS_ERROR : STATUS_OK;

    zbuf_free(resp);
    return ret;
}

/*
 * Read Coils
 */
status_t modbus_read_coils(modbus_client_t *client, uint8_t slave,
                           uint16_t addr, uint16_t count, uint8_t *result)
{
    uint8_t req_data[4];
    req_data[0] = addr >> 8;
    req_data[1] = addr & 0xFF;
    req_data[2] = count >> 8;
    req_data[3] = count & 0xFF;

    zbuf_t *req = modbus_build_request(client, slave,
                                        MODBUS_FC_READ_COILS,
                                        req_data, 4);
    if (req == NULL) return STATUS_NO_MEM;

    status_t ret = modbus_send_request(client, req);
    if (ret != STATUS_OK) return ret;

    zbuf_t *resp = modbus_recv_response(client);
    if (resp == NULL) return STATUS_TIMEOUT;

    uint8_t *data = resp->data + MODBUS_TCP_HDR_LEN;
    if (data[0] & 0x80) {
        ret = STATUS_ERROR;
    } else {
        uint8_t byte_count = data[1];
        for (uint8_t i = 0; i < byte_count; i++) {
            result[i] = data[2 + i];
        }
        ret = STATUS_OK;
    }

    zbuf_free(resp);
    return ret;
}

/*
 * Write Single Coil
 */
status_t modbus_write_single_coil(modbus_client_t *client, uint8_t slave,
                                   uint16_t addr, bool value)
{
    uint8_t req_data[4];
    req_data[0] = addr >> 8;
    req_data[1] = addr & 0xFF;
    req_data[2] = value ? 0xFF : 0x00;
    req_data[3] = 0x00;

    zbuf_t *req = modbus_build_request(client, slave,
                                        MODBUS_FC_WRITE_SINGLE_COIL,
                                        req_data, 4);
    if (req == NULL) return STATUS_NO_MEM;

    status_t ret = modbus_send_request(client, req);
    if (ret != STATUS_OK) return ret;

    zbuf_t *resp = modbus_recv_response(client);
    if (resp == NULL) return STATUS_TIMEOUT;

    uint8_t *data = resp->data + MODBUS_TCP_HDR_LEN;
    ret = (data[0] & 0x80) ? STATUS_ERROR : STATUS_OK;

    zbuf_free(resp);
    return ret;
}
