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
 * Modbus Protocol Unit Tests
 */

#include "test_framework.h"
#include "modbus.h"

static modbus_server_t test_server;
static uint16_t test_holding_regs[100];
static uint16_t test_input_regs[100];
static uint8_t test_coils[100];
static uint8_t test_discrete[100];

static void modbus_test_setup(void)
{
    /* Initialize test registers */
    for (int i = 0; i < 100; i++) {
        test_holding_regs[i] = i * 10;
        test_input_regs[i] = i * 20;
        test_coils[i] = (i % 2);
        test_discrete[i] = ((i + 1) % 2);
    }

    modbus_server_init(&test_server, 1);  /* Unit ID 1 */

    /* Set up data model */
    test_server.holding_regs = test_holding_regs;
    test_server.holding_reg_count = 100;
    test_server.input_regs = test_input_regs;
    test_server.input_reg_count = 100;
    test_server.coils = test_coils;
    test_server.coil_count = 100;
    test_server.discrete_inputs = test_discrete;
    test_server.discrete_count = 100;
}

/*
 * Test: Modbus CRC calculation
 */
TEST_CASE(modbus_crc)
{
    uint8_t data[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
    uint16_t crc = modbus_crc16(data, 6);

    /* CRC should be non-zero */
    TEST_ASSERT_NE(crc, 0);

    return TEST_PASS;
}

/*
 * Test: Read holding registers (FC03)
 */
TEST_CASE(modbus_read_holding)
{
    /* Build request */
    uint8_t request[8] = {
        0x01,       /* Unit ID */
        0x03,       /* Function code: Read Holding Registers */
        0x00, 0x00, /* Start address: 0 */
        0x00, 0x05, /* Quantity: 5 */
        0x00, 0x00  /* CRC placeholder */
    };

    uint8_t response[256];
    int resp_len = modbus_process_request(&test_server, request, 6, response);

    TEST_ASSERT(resp_len > 0);
    TEST_ASSERT_EQ(response[0], 0x01);  /* Unit ID */
    TEST_ASSERT_EQ(response[1], 0x03);  /* Function code */
    TEST_ASSERT_EQ(response[2], 10);    /* Byte count (5 regs * 2) */

    /* Check register values */
    TEST_ASSERT_EQ((response[3] << 8) | response[4], 0);    /* Reg 0 */
    TEST_ASSERT_EQ((response[5] << 8) | response[6], 10);   /* Reg 1 */
    TEST_ASSERT_EQ((response[7] << 8) | response[8], 20);   /* Reg 2 */

    return TEST_PASS;
}

/*
 * Test: Read input registers (FC04)
 */
TEST_CASE(modbus_read_input)
{
    uint8_t request[8] = {
        0x01,       /* Unit ID */
        0x04,       /* Function code: Read Input Registers */
        0x00, 0x00, /* Start address: 0 */
        0x00, 0x03, /* Quantity: 3 */
        0x00, 0x00
    };

    uint8_t response[256];
    int resp_len = modbus_process_request(&test_server, request, 6, response);

    TEST_ASSERT(resp_len > 0);
    TEST_ASSERT_EQ(response[0], 0x01);
    TEST_ASSERT_EQ(response[1], 0x04);
    TEST_ASSERT_EQ(response[2], 6);     /* Byte count */

    /* Check register values */
    TEST_ASSERT_EQ((response[3] << 8) | response[4], 0);    /* Reg 0 */
    TEST_ASSERT_EQ((response[5] << 8) | response[6], 20);   /* Reg 1 */
    TEST_ASSERT_EQ((response[7] << 8) | response[8], 40);   /* Reg 2 */

    return TEST_PASS;
}

/*
 * Test: Write single register (FC06)
 */
TEST_CASE(modbus_write_single_reg)
{
    uint8_t request[8] = {
        0x01,       /* Unit ID */
        0x06,       /* Function code: Write Single Register */
        0x00, 0x05, /* Address: 5 */
        0x12, 0x34, /* Value: 0x1234 */
        0x00, 0x00
    };

    uint8_t response[256];
    int resp_len = modbus_process_request(&test_server, request, 6, response);

    TEST_ASSERT(resp_len > 0);
    TEST_ASSERT_EQ(response[0], 0x01);
    TEST_ASSERT_EQ(response[1], 0x06);
    TEST_ASSERT_EQ((response[2] << 8) | response[3], 5);    /* Address echo */
    TEST_ASSERT_EQ((response[4] << 8) | response[5], 0x1234);  /* Value echo */

    /* Verify register was written */
    TEST_ASSERT_EQ(test_holding_regs[5], 0x1234);

    return TEST_PASS;
}

/*
 * Test: Write multiple registers (FC16)
 */
TEST_CASE(modbus_write_multiple_regs)
{
    uint8_t request[15] = {
        0x01,       /* Unit ID */
        0x10,       /* Function code: Write Multiple Registers */
        0x00, 0x0A, /* Start address: 10 */
        0x00, 0x03, /* Quantity: 3 */
        0x06,       /* Byte count */
        0x00, 0x01, /* Value 1 */
        0x00, 0x02, /* Value 2 */
        0x00, 0x03, /* Value 3 */
        0x00, 0x00
    };

    uint8_t response[256];
    int resp_len = modbus_process_request(&test_server, request, 13, response);

    TEST_ASSERT(resp_len > 0);
    TEST_ASSERT_EQ(response[0], 0x01);
    TEST_ASSERT_EQ(response[1], 0x10);
    TEST_ASSERT_EQ((response[2] << 8) | response[3], 10);   /* Start address */
    TEST_ASSERT_EQ((response[4] << 8) | response[5], 3);    /* Quantity */

    /* Verify registers were written */
    TEST_ASSERT_EQ(test_holding_regs[10], 1);
    TEST_ASSERT_EQ(test_holding_regs[11], 2);
    TEST_ASSERT_EQ(test_holding_regs[12], 3);

    return TEST_PASS;
}

/*
 * Test: Read coils (FC01)
 */
TEST_CASE(modbus_read_coils)
{
    uint8_t request[8] = {
        0x01,       /* Unit ID */
        0x01,       /* Function code: Read Coils */
        0x00, 0x00, /* Start address: 0 */
        0x00, 0x08, /* Quantity: 8 */
        0x00, 0x00
    };

    uint8_t response[256];
    int resp_len = modbus_process_request(&test_server, request, 6, response);

    TEST_ASSERT(resp_len > 0);
    TEST_ASSERT_EQ(response[0], 0x01);
    TEST_ASSERT_EQ(response[1], 0x01);
    TEST_ASSERT_EQ(response[2], 1);     /* Byte count */

    /* Coils pattern: 0,1,0,1,0,1,0,1 = 0xAA */
    TEST_ASSERT_EQ(response[3], 0xAA);

    return TEST_PASS;
}

/*
 * Test: Write single coil (FC05)
 */
TEST_CASE(modbus_write_single_coil)
{
    /* Write coil ON */
    uint8_t request[8] = {
        0x01,       /* Unit ID */
        0x05,       /* Function code: Write Single Coil */
        0x00, 0x10, /* Address: 16 */
        0xFF, 0x00, /* ON */
        0x00, 0x00
    };

    uint8_t response[256];
    int resp_len = modbus_process_request(&test_server, request, 6, response);

    TEST_ASSERT(resp_len > 0);
    TEST_ASSERT_EQ(response[0], 0x01);
    TEST_ASSERT_EQ(response[1], 0x05);

    /* Verify coil was set */
    TEST_ASSERT_EQ(test_coils[16], 1);

    return TEST_PASS;
}

/*
 * Test: Invalid function code exception
 */
TEST_CASE(modbus_exception_invalid_fc)
{
    uint8_t request[8] = {
        0x01,       /* Unit ID */
        0x99,       /* Invalid function code */
        0x00, 0x00,
        0x00, 0x01,
        0x00, 0x00
    };

    uint8_t response[256];
    int resp_len = modbus_process_request(&test_server, request, 6, response);

    TEST_ASSERT(resp_len > 0);
    TEST_ASSERT_EQ(response[0], 0x01);
    TEST_ASSERT_EQ(response[1], 0x99 | 0x80);  /* Exception response */
    TEST_ASSERT_EQ(response[2], MODBUS_EXC_ILLEGAL_FUNCTION);

    return TEST_PASS;
}

/*
 * Test: Address out of range exception
 */
TEST_CASE(modbus_exception_invalid_addr)
{
    uint8_t request[8] = {
        0x01,       /* Unit ID */
        0x03,       /* Read Holding Registers */
        0xFF, 0x00, /* Start address: 65280 (out of range) */
        0x00, 0x01, /* Quantity: 1 */
        0x00, 0x00
    };

    uint8_t response[256];
    int resp_len = modbus_process_request(&test_server, request, 6, response);

    TEST_ASSERT(resp_len > 0);
    TEST_ASSERT_EQ(response[0], 0x01);
    TEST_ASSERT_EQ(response[1], 0x03 | 0x80);  /* Exception response */
    TEST_ASSERT_EQ(response[2], MODBUS_EXC_ILLEGAL_ADDRESS);

    return TEST_PASS;
}

/*
 * Test Suite Definition
 */
static test_case_t modbus_tests[] = {
    { "modbus_crc", test_modbus_crc },
    { "modbus_read_holding", test_modbus_read_holding },
    { "modbus_read_input", test_modbus_read_input },
    { "modbus_write_single_reg", test_modbus_write_single_reg },
    { "modbus_write_multiple_regs", test_modbus_write_multiple_regs },
    { "modbus_read_coils", test_modbus_read_coils },
    { "modbus_write_single_coil", test_modbus_write_single_coil },
    { "modbus_exception_invalid_fc", test_modbus_exception_invalid_fc },
    { "modbus_exception_invalid_addr", test_modbus_exception_invalid_addr },
};

test_suite_t modbus_test_suite = {
    .name = "Modbus Protocol",
    .tests = modbus_tests,
    .test_count = sizeof(modbus_tests) / sizeof(test_case_t),
    .setup = modbus_test_setup,
    .teardown = NULL
};
