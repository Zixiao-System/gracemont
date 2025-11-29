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
 * Zero-Copy PROFINET RT Protocol Implementation
 */

#ifndef PROFINET_H
#define PROFINET_H

#include "rtos_types.h"
#include "zbuf.h"
#include "net_stack.h"

/* PROFINET Ethernet Types */
#define ETH_TYPE_PROFINET       0x8892
#define ETH_TYPE_PROFINET_RT    0x8892
#define ETH_TYPE_LLDP           0x88CC
#define ETH_TYPE_MRP            0x88E3

/* PROFINET Frame IDs */
#define PNIO_FRAME_ID_RT_MIN        0x8000
#define PNIO_FRAME_ID_RT_MAX        0xBFFF
#define PNIO_FRAME_ID_ALARM_HIGH    0xFC01
#define PNIO_FRAME_ID_ALARM_LOW     0xFE01
#define PNIO_FRAME_ID_RTC3_MIN      0x0100
#define PNIO_FRAME_ID_RTC3_MAX      0x7FFF
#define PNIO_FRAME_ID_DCP           0xFEFC
#define PNIO_FRAME_ID_DCP_HELLO     0xFEFD
#define PNIO_FRAME_ID_DCP_GET       0xFEFE
#define PNIO_FRAME_ID_DCP_SET       0xFEFF

/* PROFINET Service IDs */
#define PNIO_SERVICE_CONNECT        0x01
#define PNIO_SERVICE_RELEASE        0x02
#define PNIO_SERVICE_READ           0x03
#define PNIO_SERVICE_WRITE          0x04
#define PNIO_SERVICE_CONTROL        0x05

/* DCP Service IDs */
#define DCP_SERVICE_GET             0x03
#define DCP_SERVICE_SET             0x04
#define DCP_SERVICE_IDENTIFY        0x05
#define DCP_SERVICE_HELLO           0x06

/* DCP Option */
#define DCP_OPT_IP                  0x01
#define DCP_OPT_DEVICE              0x02
#define DCP_OPT_DHCP                0x03
#define DCP_OPT_CONTROL             0x05
#define DCP_OPT_ALL                 0xFF

/* DCP Sub-options */
#define DCP_SUBOPT_IP_MAC           0x01
#define DCP_SUBOPT_IP_PARAM         0x02
#define DCP_SUBOPT_IP_FULL          0x03
#define DCP_SUBOPT_DEV_VENDOR       0x01
#define DCP_SUBOPT_DEV_NAME         0x02
#define DCP_SUBOPT_DEV_ID           0x03
#define DCP_SUBOPT_DEV_ROLE         0x04
#define DCP_SUBOPT_DEV_OPTIONS      0x05
#define DCP_SUBOPT_DEV_INSTANCE     0x07

/* PROFINET RT Header */
typedef struct PACKED {
    uint16_t    frame_id;
} pnio_rt_hdr_t;

/* PROFINET Data Status */
typedef struct PACKED {
    uint8_t     status;
    uint8_t     transfer_status;
} pnio_data_status_t;

#define PNIO_STATUS_PRIMARY         0x01
#define PNIO_STATUS_VALID           0x04
#define PNIO_STATUS_STATE           0x10
#define PNIO_STATUS_PROBLEM         0x20

/* DCP Header */
typedef struct PACKED {
    uint8_t     service_id;
    uint8_t     service_type;
    uint32_t    xid;
    uint16_t    response_delay;
    uint16_t    data_length;
} dcp_hdr_t;

#define DCP_SERVICE_TYPE_REQUEST    0x00
#define DCP_SERVICE_TYPE_RESPONSE   0x01

/* Slot/Subslot Definition */
typedef struct {
    uint16_t    slot_number;
    uint16_t    subslot_number;
    uint32_t    module_ident;
    uint32_t    submodule_ident;

    /* I/O Data */
    uint8_t     *input_data;
    uint16_t    input_length;
    uint8_t     *output_data;
    uint16_t    output_length;

    /* IOCS/IOPS */
    uint8_t     iocs;           /* IO Consumer Status */
    uint8_t     iops;           /* IO Provider Status */

    bool        plugged;
} pnio_subslot_t;

typedef struct {
    uint16_t        slot_number;
    uint32_t        module_ident;
    pnio_subslot_t  subslots[CONFIG_PROFINET_MAX_SUBSLOTS];
    uint16_t        subslot_count;
    bool            plugged;
} pnio_slot_t;

/* AR (Application Relationship) */
typedef struct {
    uint8_t     ar_uuid[16];
    uint32_t    ar_properties;
    uint16_t    ar_type;
    uint16_t    session_key;
    bool        active;

    /* IOCR (IO Communication Relation) */
    uint16_t    input_frame_id;
    uint16_t    output_frame_id;
    uint32_t    send_clock;
    uint32_t    reduction_ratio;
    uint32_t    phase;

    /* Peer info */
    uint8_t     peer_mac[6];
} pnio_ar_t;

/* Alarm */
typedef struct {
    uint16_t    alarm_type;
    uint16_t    slot;
    uint16_t    subslot;
    uint32_t    module_ident;
    uint32_t    submodule_ident;
    uint16_t    sequence_number;
    uint8_t     alarm_specifier;
    uint8_t     *data;
    uint16_t    data_length;
} pnio_alarm_t;

/* PROFINET Device Context */
typedef struct {
    /* Identity */
    char            name_of_station[64];
    uint16_t        vendor_id;
    uint16_t        device_id;
    uint8_t         device_role;
    uint16_t        instance_high;
    uint16_t        instance_low;

    /* Network */
    netif_t         *netif;
    uint32_t        ip_addr;
    uint32_t        netmask;
    uint32_t        gateway;

    /* Slots */
    pnio_slot_t     slots[CONFIG_PROFINET_MAX_SLOTS];
    uint16_t        slot_count;

    /* ARs */
    pnio_ar_t       ar[CONFIG_PROFINET_MAX_DEVICES];
    uint16_t        ar_count;

    /* RT Communication */
    bool            running;
    uint32_t        cycle_time_us;
    uint64_t        cycle_count;
    uint64_t        last_cycle_time;

    /* Cyclic data buffers (zero-copy) */
    zbuf_t          *tx_buffer;
    zbuf_t          *rx_buffer;

    /* Callbacks */
    void            (*on_connect)(pnio_ar_t *ar);
    void            (*on_disconnect)(pnio_ar_t *ar);
    void            (*on_data_received)(uint16_t slot, uint16_t subslot);
    void            (*on_alarm)(pnio_alarm_t *alarm);
    status_t        (*on_read)(uint16_t slot, uint16_t subslot, uint16_t index,
                               uint8_t *data, uint16_t *length);
    status_t        (*on_write)(uint16_t slot, uint16_t subslot, uint16_t index,
                                uint8_t *data, uint16_t length);

    spinlock_t      lock;
} pnio_device_t;

/* PROFINET Controller Context */
typedef struct {
    netif_t         *netif;
    pnio_ar_t       ar;
    bool            connected;
    uint32_t        cycle_time_us;

    /* Target device */
    char            target_name[64];
    uint8_t         target_mac[6];
} pnio_controller_t;

/* API Functions */

/* Device (IO-Device) */
status_t pnio_device_init(pnio_device_t *dev, netif_t *netif,
                          const char *name, uint16_t vendor_id, uint16_t device_id);
status_t pnio_device_start(pnio_device_t *dev);
void pnio_device_stop(pnio_device_t *dev);
void pnio_device_poll(pnio_device_t *dev);

/* Slot Management */
status_t pnio_add_slot(pnio_device_t *dev, uint16_t slot_number, uint32_t module_ident);
status_t pnio_add_subslot(pnio_device_t *dev, uint16_t slot_number,
                          uint16_t subslot_number, uint32_t submodule_ident,
                          uint16_t input_length, uint16_t output_length);
status_t pnio_plug_submodule(pnio_device_t *dev, uint16_t slot, uint16_t subslot);
status_t pnio_pull_submodule(pnio_device_t *dev, uint16_t slot, uint16_t subslot);

/* Data Access (Zero-Copy) */
uint8_t *pnio_get_input_data(pnio_device_t *dev, uint16_t slot, uint16_t subslot);
uint8_t *pnio_get_output_data(pnio_device_t *dev, uint16_t slot, uint16_t subslot);
void pnio_set_iops(pnio_device_t *dev, uint16_t slot, uint16_t subslot, uint8_t iops);
uint8_t pnio_get_iocs(pnio_device_t *dev, uint16_t slot, uint16_t subslot);

/* Alarms */
status_t pnio_send_alarm(pnio_device_t *dev, pnio_alarm_t *alarm);
status_t pnio_send_diag_alarm(pnio_device_t *dev, uint16_t slot, uint16_t subslot,
                               uint16_t channel, uint16_t error_type);

/* DCP */
void pnio_dcp_input(pnio_device_t *dev, zbuf_t *zb);
status_t pnio_dcp_identify(pnio_device_t *dev, const char *name);

/* RT Frame Processing */
void pnio_rt_input(pnio_device_t *dev, zbuf_t *zb);
status_t pnio_rt_send(pnio_device_t *dev);

/* Controller API */
status_t pnio_controller_init(pnio_controller_t *ctrl, netif_t *netif);
status_t pnio_controller_connect(pnio_controller_t *ctrl, const char *device_name);
void pnio_controller_disconnect(pnio_controller_t *ctrl);
status_t pnio_controller_read(pnio_controller_t *ctrl, uint16_t slot, uint16_t subslot,
                               uint16_t index, uint8_t *data, uint16_t *length);
status_t pnio_controller_write(pnio_controller_t *ctrl, uint16_t slot, uint16_t subslot,
                                uint16_t index, uint8_t *data, uint16_t length);

#endif /* PROFINET_H */
