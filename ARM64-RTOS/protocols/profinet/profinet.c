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
 * Zero-Copy PROFINET RT Implementation
 */

#include "profinet.h"
#include "rtos_config.h"

/* PROFINET Multicast Addresses */
static const uint8_t pnio_mc_rt[6] = {0x01, 0x0E, 0xCF, 0x00, 0x00, 0x00};
static const uint8_t pnio_mc_dcp[6] = {0x01, 0x0E, 0xCF, 0x00, 0x00, 0x00};

/*
 * Device Initialization
 */
status_t pnio_device_init(pnio_device_t *dev, netif_t *netif,
                          const char *name, uint16_t vendor_id, uint16_t device_id)
{
    /* Copy name */
    int i = 0;
    while (name[i] && i < 63) {
        dev->name_of_station[i] = name[i];
        i++;
    }
    dev->name_of_station[i] = '\0';

    dev->vendor_id = vendor_id;
    dev->device_id = device_id;
    dev->device_role = 0x01;  /* IO-Device */
    dev->instance_high = 0;
    dev->instance_low = 1;

    dev->netif = netif;
    dev->ip_addr = netif->ip;
    dev->netmask = netif->netmask;
    dev->gateway = netif->gateway;

    dev->slot_count = 0;
    dev->ar_count = 0;
    dev->running = false;
    dev->cycle_time_us = CONFIG_PROFINET_CYCLE_TIME;
    dev->cycle_count = 0;
    dev->last_cycle_time = 0;

    dev->tx_buffer = NULL;
    dev->rx_buffer = NULL;

    dev->on_connect = NULL;
    dev->on_disconnect = NULL;
    dev->on_data_received = NULL;
    dev->on_alarm = NULL;
    dev->on_read = NULL;
    dev->on_write = NULL;

    dev->lock = (spinlock_t)SPINLOCK_INIT;

    return STATUS_OK;
}

/*
 * Slot Management
 */
status_t pnio_add_slot(pnio_device_t *dev, uint16_t slot_number, uint32_t module_ident)
{
    if (dev->slot_count >= CONFIG_PROFINET_MAX_SLOTS) {
        return STATUS_NO_MEM;
    }

    spin_lock(&dev->lock);

    pnio_slot_t *slot = &dev->slots[dev->slot_count++];
    slot->slot_number = slot_number;
    slot->module_ident = module_ident;
    slot->subslot_count = 0;
    slot->plugged = false;

    spin_unlock(&dev->lock);
    return STATUS_OK;
}

status_t pnio_add_subslot(pnio_device_t *dev, uint16_t slot_number,
                          uint16_t subslot_number, uint32_t submodule_ident,
                          uint16_t input_length, uint16_t output_length)
{
    spin_lock(&dev->lock);

    /* Find slot */
    pnio_slot_t *slot = NULL;
    for (uint16_t i = 0; i < dev->slot_count; i++) {
        if (dev->slots[i].slot_number == slot_number) {
            slot = &dev->slots[i];
            break;
        }
    }

    if (slot == NULL || slot->subslot_count >= CONFIG_PROFINET_MAX_SUBSLOTS) {
        spin_unlock(&dev->lock);
        return STATUS_ERROR;
    }

    pnio_subslot_t *subslot = &slot->subslots[slot->subslot_count++];
    subslot->slot_number = slot_number;
    subslot->subslot_number = subslot_number;
    subslot->module_ident = slot->module_ident;
    subslot->submodule_ident = submodule_ident;
    subslot->input_length = input_length;
    subslot->output_length = output_length;

    /* Allocate data buffers */
    if (input_length > 0) {
        subslot->input_data = heap_alloc(input_length);
        for (uint16_t i = 0; i < input_length; i++) {
            subslot->input_data[i] = 0;
        }
    } else {
        subslot->input_data = NULL;
    }

    if (output_length > 0) {
        subslot->output_data = heap_alloc(output_length);
        for (uint16_t i = 0; i < output_length; i++) {
            subslot->output_data[i] = 0;
        }
    } else {
        subslot->output_data = NULL;
    }

    subslot->iocs = 0x80;  /* Good */
    subslot->iops = 0x80;  /* Good */
    subslot->plugged = false;

    spin_unlock(&dev->lock);
    return STATUS_OK;
}

status_t pnio_plug_submodule(pnio_device_t *dev, uint16_t slot_num, uint16_t subslot_num)
{
    spin_lock(&dev->lock);

    for (uint16_t i = 0; i < dev->slot_count; i++) {
        pnio_slot_t *slot = &dev->slots[i];
        if (slot->slot_number == slot_num) {
            slot->plugged = true;
            for (uint16_t j = 0; j < slot->subslot_count; j++) {
                if (slot->subslots[j].subslot_number == subslot_num) {
                    slot->subslots[j].plugged = true;
                    spin_unlock(&dev->lock);
                    return STATUS_OK;
                }
            }
        }
    }

    spin_unlock(&dev->lock);
    return STATUS_ERROR;
}

status_t pnio_pull_submodule(pnio_device_t *dev, uint16_t slot_num, uint16_t subslot_num)
{
    spin_lock(&dev->lock);

    for (uint16_t i = 0; i < dev->slot_count; i++) {
        pnio_slot_t *slot = &dev->slots[i];
        if (slot->slot_number == slot_num) {
            for (uint16_t j = 0; j < slot->subslot_count; j++) {
                if (slot->subslots[j].subslot_number == subslot_num) {
                    slot->subslots[j].plugged = false;
                    spin_unlock(&dev->lock);
                    return STATUS_OK;
                }
            }
        }
    }

    spin_unlock(&dev->lock);
    return STATUS_ERROR;
}

/*
 * Data Access (Zero-Copy)
 */
uint8_t *pnio_get_input_data(pnio_device_t *dev, uint16_t slot_num, uint16_t subslot_num)
{
    for (uint16_t i = 0; i < dev->slot_count; i++) {
        pnio_slot_t *slot = &dev->slots[i];
        if (slot->slot_number == slot_num) {
            for (uint16_t j = 0; j < slot->subslot_count; j++) {
                if (slot->subslots[j].subslot_number == subslot_num) {
                    return slot->subslots[j].input_data;
                }
            }
        }
    }
    return NULL;
}

uint8_t *pnio_get_output_data(pnio_device_t *dev, uint16_t slot_num, uint16_t subslot_num)
{
    for (uint16_t i = 0; i < dev->slot_count; i++) {
        pnio_slot_t *slot = &dev->slots[i];
        if (slot->slot_number == slot_num) {
            for (uint16_t j = 0; j < slot->subslot_count; j++) {
                if (slot->subslots[j].subslot_number == subslot_num) {
                    return slot->subslots[j].output_data;
                }
            }
        }
    }
    return NULL;
}

void pnio_set_iops(pnio_device_t *dev, uint16_t slot_num, uint16_t subslot_num, uint8_t iops)
{
    for (uint16_t i = 0; i < dev->slot_count; i++) {
        pnio_slot_t *slot = &dev->slots[i];
        if (slot->slot_number == slot_num) {
            for (uint16_t j = 0; j < slot->subslot_count; j++) {
                if (slot->subslots[j].subslot_number == subslot_num) {
                    slot->subslots[j].iops = iops;
                    return;
                }
            }
        }
    }
}

uint8_t pnio_get_iocs(pnio_device_t *dev, uint16_t slot_num, uint16_t subslot_num)
{
    for (uint16_t i = 0; i < dev->slot_count; i++) {
        pnio_slot_t *slot = &dev->slots[i];
        if (slot->slot_number == slot_num) {
            for (uint16_t j = 0; j < slot->subslot_count; j++) {
                if (slot->subslots[j].subslot_number == subslot_num) {
                    return slot->subslots[j].iocs;
                }
            }
        }
    }
    return 0;
}

/*
 * DCP Frame Processing
 */
void pnio_dcp_input(pnio_device_t *dev, zbuf_t *zb)
{
    if (zb->len < sizeof(dcp_hdr_t)) {
        zbuf_free(zb);
        return;
    }

    dcp_hdr_t *dcp = (dcp_hdr_t *)(zb->data + 2);  /* Skip frame ID */
    uint8_t service_id = dcp->service_id;
    uint8_t service_type = dcp->service_type;
    uint32_t xid = ntohl(dcp->xid);
    uint16_t data_len = ntohs(dcp->data_length);

    uint8_t *data = (uint8_t *)(dcp + 1);

    if (service_id == DCP_SERVICE_IDENTIFY && service_type == DCP_SERVICE_TYPE_REQUEST) {
        /* Check if name matches */
        bool match = true;

        /* Parse request blocks */
        uint16_t offset = 0;
        while (offset < data_len) {
            uint8_t option = data[offset];
            uint8_t suboption = data[offset + 1];
            uint16_t block_len = (data[offset + 2] << 8) | data[offset + 3];

            if (option == DCP_OPT_DEVICE && suboption == DCP_SUBOPT_DEV_NAME) {
                /* Check name */
                char *req_name = (char *)&data[offset + 4];
                int i = 0;
                while (i < block_len && dev->name_of_station[i]) {
                    if (req_name[i] != dev->name_of_station[i]) {
                        match = false;
                        break;
                    }
                    i++;
                }
                if (i < block_len && req_name[i] != '\0') {
                    match = false;
                }
            } else if (option == DCP_OPT_ALL) {
                /* Match all */
            }

            offset += 4 + block_len;
            if (block_len & 1) offset++;  /* Padding */
        }

        if (match) {
            /* Send Identify Response */
            zbuf_t *resp = zbuf_alloc_tx(256);
            if (resp == NULL) {
                zbuf_free(zb);
                return;
            }

            uint8_t *p = zbuf_put(resp, 0);

            /* Frame ID */
            *p++ = (PNIO_FRAME_ID_DCP >> 8) & 0xFF;
            *p++ = PNIO_FRAME_ID_DCP & 0xFF;

            /* DCP Header */
            *p++ = DCP_SERVICE_IDENTIFY;
            *p++ = DCP_SERVICE_TYPE_RESPONSE;
            *p++ = (xid >> 24) & 0xFF;
            *p++ = (xid >> 16) & 0xFF;
            *p++ = (xid >> 8) & 0xFF;
            *p++ = xid & 0xFF;
            *p++ = 0;  /* Response delay */
            *p++ = 0;

            uint8_t *len_ptr = p;
            p += 2;  /* Data length placeholder */

            uint16_t total_len = 0;

            /* Device/Name block */
            *p++ = DCP_OPT_DEVICE;
            *p++ = DCP_SUBOPT_DEV_NAME;
            int name_len = 0;
            while (dev->name_of_station[name_len]) name_len++;
            *p++ = (name_len >> 8) & 0xFF;
            *p++ = name_len & 0xFF;
            for (int i = 0; i < name_len; i++) {
                *p++ = dev->name_of_station[i];
            }
            if (name_len & 1) *p++ = 0;  /* Padding */
            total_len += 4 + name_len + (name_len & 1);

            /* Device/Vendor block */
            *p++ = DCP_OPT_DEVICE;
            *p++ = DCP_SUBOPT_DEV_VENDOR;
            *p++ = 0;
            *p++ = 4;
            *p++ = (dev->vendor_id >> 8) & 0xFF;
            *p++ = dev->vendor_id & 0xFF;
            *p++ = (dev->device_id >> 8) & 0xFF;
            *p++ = dev->device_id & 0xFF;
            total_len += 8;

            /* IP block */
            *p++ = DCP_OPT_IP;
            *p++ = DCP_SUBOPT_IP_PARAM;
            *p++ = 0;
            *p++ = 14;
            *p++ = 0; *p++ = 0;  /* Block info */
            *p++ = (dev->ip_addr >> 24) & 0xFF;
            *p++ = (dev->ip_addr >> 16) & 0xFF;
            *p++ = (dev->ip_addr >> 8) & 0xFF;
            *p++ = dev->ip_addr & 0xFF;
            *p++ = (dev->netmask >> 24) & 0xFF;
            *p++ = (dev->netmask >> 16) & 0xFF;
            *p++ = (dev->netmask >> 8) & 0xFF;
            *p++ = dev->netmask & 0xFF;
            *p++ = (dev->gateway >> 24) & 0xFF;
            *p++ = (dev->gateway >> 16) & 0xFF;
            *p++ = (dev->gateway >> 8) & 0xFF;
            *p++ = dev->gateway & 0xFF;
            total_len += 18;

            /* Device/Role block */
            *p++ = DCP_OPT_DEVICE;
            *p++ = DCP_SUBOPT_DEV_ROLE;
            *p++ = 0;
            *p++ = 4;
            *p++ = 0; *p++ = 0;  /* Block info */
            *p++ = dev->device_role;
            *p++ = 0;  /* Reserved */
            total_len += 8;

            /* Update length */
            len_ptr[0] = (total_len >> 8) & 0xFF;
            len_ptr[1] = total_len & 0xFF;

            resp->tail = p;
            resp->len = p - resp->data;

            /* Send response via Ethernet */
            eth_hdr_t *eth_req = (eth_hdr_t *)(zb->data - ETH_HDR_LEN - 2);
            dev->netif->send(dev->netif, resp);
        }
    } else if (service_id == DCP_SERVICE_SET && service_type == DCP_SERVICE_TYPE_REQUEST) {
        /* Handle Set requests */
        uint16_t offset = 0;
        while (offset < data_len) {
            uint8_t option = data[offset];
            uint8_t suboption = data[offset + 1];
            uint16_t block_len = (data[offset + 2] << 8) | data[offset + 3];

            if (option == DCP_OPT_IP && suboption == DCP_SUBOPT_IP_PARAM) {
                /* Set IP parameters */
                uint8_t *ip_data = &data[offset + 6];  /* Skip block info */
                dev->ip_addr = (ip_data[0] << 24) | (ip_data[1] << 16) |
                               (ip_data[2] << 8) | ip_data[3];
                dev->netmask = (ip_data[4] << 24) | (ip_data[5] << 16) |
                               (ip_data[6] << 8) | ip_data[7];
                dev->gateway = (ip_data[8] << 24) | (ip_data[9] << 16) |
                               (ip_data[10] << 8) | ip_data[11];
            } else if (option == DCP_OPT_DEVICE && suboption == DCP_SUBOPT_DEV_NAME) {
                /* Set name */
                char *new_name = (char *)&data[offset + 4];
                for (int i = 0; i < block_len && i < 63; i++) {
                    dev->name_of_station[i] = new_name[i];
                }
                dev->name_of_station[block_len < 63 ? block_len : 63] = '\0';
            }

            offset += 4 + block_len;
            if (block_len & 1) offset++;
        }

        /* Send Set Response */
        /* ... similar to Identify response ... */
    }

    zbuf_free(zb);
}

/*
 * RT Frame Processing (Zero-Copy)
 */
void pnio_rt_input(pnio_device_t *dev, zbuf_t *zb)
{
    if (zb->len < 2) {
        zbuf_free(zb);
        return;
    }

    uint16_t frame_id = (zb->data[0] << 8) | zb->data[1];

    /* Check if it's for our AR */
    pnio_ar_t *ar = NULL;
    for (uint16_t i = 0; i < dev->ar_count; i++) {
        if (dev->ar[i].active && dev->ar[i].output_frame_id == frame_id) {
            ar = &dev->ar[i];
            break;
        }
    }

    if (ar == NULL) {
        /* Check DCP frames */
        if (frame_id >= PNIO_FRAME_ID_DCP && frame_id <= PNIO_FRAME_ID_DCP_SET) {
            pnio_dcp_input(dev, zb);
            return;
        }
        zbuf_free(zb);
        return;
    }

    /* Record timestamp for jitter analysis */
    zb->timestamp = get_system_ticks();

    /* Process cyclic data */
    uint8_t *data = zb->data + 2;  /* Skip frame ID */
    uint16_t offset = 0;

    spin_lock(&dev->lock);

    /* Copy output data to subslots */
    for (uint16_t i = 0; i < dev->slot_count; i++) {
        pnio_slot_t *slot = &dev->slots[i];
        for (uint16_t j = 0; j < slot->subslot_count; j++) {
            pnio_subslot_t *subslot = &slot->subslots[j];
            if (subslot->plugged && subslot->output_length > 0) {
                /* Copy output data (zero-copy from zbuf) */
                for (uint16_t k = 0; k < subslot->output_length; k++) {
                    subslot->output_data[k] = data[offset + k];
                }
                offset += subslot->output_length;

                /* Read IOPS from controller */
                subslot->iocs = data[offset++];

                /* Notify application */
                if (dev->on_data_received) {
                    dev->on_data_received(slot->slot_number, subslot->subslot_number);
                }
            }
        }
    }

    spin_unlock(&dev->lock);

    /* Store for reference */
    if (dev->rx_buffer != NULL) {
        zbuf_free(dev->rx_buffer);
    }
    dev->rx_buffer = zb;

    dev->cycle_count++;
}

/*
 * RT Frame Sending (Zero-Copy)
 */
status_t pnio_rt_send(pnio_device_t *dev)
{
    if (dev->ar_count == 0) {
        return STATUS_ERROR;
    }

    pnio_ar_t *ar = &dev->ar[0];  /* Use first active AR */
    if (!ar->active) {
        return STATUS_ERROR;
    }

    /* Calculate total data length */
    uint16_t data_len = 2;  /* Frame ID */
    for (uint16_t i = 0; i < dev->slot_count; i++) {
        pnio_slot_t *slot = &dev->slots[i];
        for (uint16_t j = 0; j < slot->subslot_count; j++) {
            pnio_subslot_t *subslot = &slot->subslots[j];
            if (subslot->plugged) {
                data_len += subslot->input_length + 1;  /* Data + IOPS */
            }
        }
    }
    data_len += 4;  /* Cycle counter + Data status + Transfer status */

    /* Allocate TX buffer */
    zbuf_t *zb = zbuf_alloc_tx(data_len + ETH_HDR_LEN);
    if (zb == NULL) {
        return STATUS_NO_MEM;
    }

    uint8_t *p = zbuf_put(zb, data_len);

    /* Frame ID */
    *p++ = (ar->input_frame_id >> 8) & 0xFF;
    *p++ = ar->input_frame_id & 0xFF;

    spin_lock(&dev->lock);

    /* Build cyclic data */
    for (uint16_t i = 0; i < dev->slot_count; i++) {
        pnio_slot_t *slot = &dev->slots[i];
        for (uint16_t j = 0; j < slot->subslot_count; j++) {
            pnio_subslot_t *subslot = &slot->subslots[j];
            if (subslot->plugged && subslot->input_length > 0) {
                /* Copy input data */
                for (uint16_t k = 0; k < subslot->input_length; k++) {
                    *p++ = subslot->input_data[k];
                }
                /* Add IOPS */
                *p++ = subslot->iops;
            }
        }
    }

    spin_unlock(&dev->lock);

    /* Cycle counter */
    uint16_t cycle = (uint16_t)(dev->cycle_count & 0xFFFF);
    *p++ = (cycle >> 8) & 0xFF;
    *p++ = cycle & 0xFF;

    /* Data status */
    *p++ = PNIO_STATUS_PRIMARY | PNIO_STATUS_VALID | PNIO_STATUS_STATE;

    /* Transfer status */
    *p++ = 0;

    /* Push Ethernet header */
    eth_hdr_t *eth = (eth_hdr_t *)zbuf_push(zb, ETH_HDR_LEN);
    for (int i = 0; i < 6; i++) {
        eth->dst[i] = ar->peer_mac[i];
        eth->src[i] = dev->netif->mac[i];
    }
    eth->type = htons(ETH_TYPE_PROFINET);

    /* Send via network interface */
    status_t ret = dev->netif->send(dev->netif, zb);

    dev->last_cycle_time = get_system_ticks();
    dev->cycle_count++;

    return ret;
}

/*
 * Device Start/Stop
 */
status_t pnio_device_start(pnio_device_t *dev)
{
    dev->running = true;
    dev->cycle_count = 0;
    return STATUS_OK;
}

void pnio_device_stop(pnio_device_t *dev)
{
    dev->running = false;

    if (dev->tx_buffer != NULL) {
        zbuf_free(dev->tx_buffer);
        dev->tx_buffer = NULL;
    }
    if (dev->rx_buffer != NULL) {
        zbuf_free(dev->rx_buffer);
        dev->rx_buffer = NULL;
    }
}

/*
 * Device Poll (called from RTOS task)
 */
void pnio_device_poll(pnio_device_t *dev)
{
    if (!dev->running) return;

    /* Check cycle time */
    tick_t now = get_system_ticks();
    tick_t elapsed = now - dev->last_cycle_time;

    if (elapsed >= (dev->cycle_time_us / 1000)) {
        /* Time to send cyclic data */
        for (uint16_t i = 0; i < dev->ar_count; i++) {
            if (dev->ar[i].active) {
                pnio_rt_send(dev);
            }
        }
    }
}

/*
 * Alarm Handling
 */
status_t pnio_send_alarm(pnio_device_t *dev, pnio_alarm_t *alarm)
{
    if (dev->ar_count == 0) {
        return STATUS_ERROR;
    }

    pnio_ar_t *ar = &dev->ar[0];
    if (!ar->active) {
        return STATUS_ERROR;
    }

    /* Allocate alarm frame */
    uint16_t frame_len = 32 + alarm->data_length;
    zbuf_t *zb = zbuf_alloc_tx(frame_len + ETH_HDR_LEN);
    if (zb == NULL) {
        return STATUS_NO_MEM;
    }

    uint8_t *p = zbuf_put(zb, frame_len);

    /* Frame ID for alarm */
    uint16_t frame_id = (alarm->alarm_type & 0x8000) ?
                        PNIO_FRAME_ID_ALARM_HIGH : PNIO_FRAME_ID_ALARM_LOW;
    *p++ = (frame_id >> 8) & 0xFF;
    *p++ = frame_id & 0xFF;

    /* Alarm PDU header */
    *p++ = 0x04;  /* Alarm PDU type */
    *p++ = 0x00;  /* Add flags */

    /* Slot/Subslot */
    *p++ = (alarm->slot >> 8) & 0xFF;
    *p++ = alarm->slot & 0xFF;
    *p++ = (alarm->subslot >> 8) & 0xFF;
    *p++ = alarm->subslot & 0xFF;

    /* Module/Submodule ident */
    *p++ = (alarm->module_ident >> 24) & 0xFF;
    *p++ = (alarm->module_ident >> 16) & 0xFF;
    *p++ = (alarm->module_ident >> 8) & 0xFF;
    *p++ = alarm->module_ident & 0xFF;
    *p++ = (alarm->submodule_ident >> 24) & 0xFF;
    *p++ = (alarm->submodule_ident >> 16) & 0xFF;
    *p++ = (alarm->submodule_ident >> 8) & 0xFF;
    *p++ = alarm->submodule_ident & 0xFF;

    /* Alarm type */
    *p++ = (alarm->alarm_type >> 8) & 0xFF;
    *p++ = alarm->alarm_type & 0xFF;

    /* Sequence number */
    *p++ = (alarm->sequence_number >> 8) & 0xFF;
    *p++ = alarm->sequence_number & 0xFF;

    /* Alarm specifier */
    *p++ = 0;
    *p++ = alarm->alarm_specifier;

    /* Alarm data */
    for (uint16_t i = 0; i < alarm->data_length; i++) {
        *p++ = alarm->data[i];
    }

    /* Push Ethernet header */
    eth_hdr_t *eth = (eth_hdr_t *)zbuf_push(zb, ETH_HDR_LEN);
    for (int i = 0; i < 6; i++) {
        eth->dst[i] = ar->peer_mac[i];
        eth->src[i] = dev->netif->mac[i];
    }
    eth->type = htons(ETH_TYPE_PROFINET);

    return dev->netif->send(dev->netif, zb);
}

status_t pnio_send_diag_alarm(pnio_device_t *dev, uint16_t slot, uint16_t subslot,
                               uint16_t channel, uint16_t error_type)
{
    pnio_alarm_t alarm;
    alarm.alarm_type = 0x0001;  /* Diagnosis alarm */
    alarm.slot = slot;
    alarm.subslot = subslot;

    /* Find module/submodule idents */
    for (uint16_t i = 0; i < dev->slot_count; i++) {
        if (dev->slots[i].slot_number == slot) {
            alarm.module_ident = dev->slots[i].module_ident;
            for (uint16_t j = 0; j < dev->slots[i].subslot_count; j++) {
                if (dev->slots[i].subslots[j].subslot_number == subslot) {
                    alarm.submodule_ident = dev->slots[i].subslots[j].submodule_ident;
                    break;
                }
            }
            break;
        }
    }

    static uint16_t seq_num = 0;
    alarm.sequence_number = seq_num++;
    alarm.alarm_specifier = 0x01;  /* Appears */

    /* Build diagnosis data */
    uint8_t diag_data[8];
    diag_data[0] = (channel >> 8) & 0xFF;
    diag_data[1] = channel & 0xFF;
    diag_data[2] = 0x80;  /* Channel properties */
    diag_data[3] = 0x00;
    diag_data[4] = (error_type >> 8) & 0xFF;
    diag_data[5] = error_type & 0xFF;
    diag_data[6] = 0x00;  /* Extended info */
    diag_data[7] = 0x00;

    alarm.data = diag_data;
    alarm.data_length = 8;

    return pnio_send_alarm(dev, &alarm);
}
