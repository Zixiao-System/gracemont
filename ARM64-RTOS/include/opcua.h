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
 * Zero-Copy OPC UA Protocol Implementation
 */

#ifndef OPCUA_H
#define OPCUA_H

#include "rtos_types.h"
#include "zbuf.h"
#include "net_stack.h"

/* OPC UA Message Types */
#define OPCUA_MSG_HEL       "HEL"   /* Hello */
#define OPCUA_MSG_ACK       "ACK"   /* Acknowledge */
#define OPCUA_MSG_ERR       "ERR"   /* Error */
#define OPCUA_MSG_OPN       "OPN"   /* OpenSecureChannel */
#define OPCUA_MSG_CLO       "CLO"   /* CloseSecureChannel */
#define OPCUA_MSG_MSG       "MSG"   /* Message */

/* OPC UA Service IDs */
#define OPCUA_SVC_FIND_SERVERS          420
#define OPCUA_SVC_GET_ENDPOINTS         426
#define OPCUA_SVC_CREATE_SESSION        459
#define OPCUA_SVC_ACTIVATE_SESSION      465
#define OPCUA_SVC_CLOSE_SESSION         471
#define OPCUA_SVC_READ                  629
#define OPCUA_SVC_WRITE                 671
#define OPCUA_SVC_BROWSE                525
#define OPCUA_SVC_CREATE_SUBSCRIPTION   781
#define OPCUA_SVC_DELETE_SUBSCRIPTION   845
#define OPCUA_SVC_PUBLISH               824
#define OPCUA_SVC_CREATE_MONITORED      743

/* OPC UA Status Codes */
#define OPCUA_STATUS_GOOD               0x00000000
#define OPCUA_STATUS_BAD                0x80000000
#define OPCUA_STATUS_UNCERTAIN          0x40000000
#define OPCUA_STATUS_BAD_NODEID_UNKNOWN 0x80340000
#define OPCUA_STATUS_BAD_ATTR_INVALID   0x80350000
#define OPCUA_STATUS_BAD_TYPE_MISMATCH  0x80360000
#define OPCUA_STATUS_BAD_TIMEOUT        0x800A0000
#define OPCUA_STATUS_BAD_SESSION        0x80060000

/* OPC UA Data Types */
typedef enum {
    OPCUA_TYPE_NULL         = 0,
    OPCUA_TYPE_BOOLEAN      = 1,
    OPCUA_TYPE_SBYTE        = 2,
    OPCUA_TYPE_BYTE         = 3,
    OPCUA_TYPE_INT16        = 4,
    OPCUA_TYPE_UINT16       = 5,
    OPCUA_TYPE_INT32        = 6,
    OPCUA_TYPE_UINT32       = 7,
    OPCUA_TYPE_INT64        = 8,
    OPCUA_TYPE_UINT64       = 9,
    OPCUA_TYPE_FLOAT        = 10,
    OPCUA_TYPE_DOUBLE       = 11,
    OPCUA_TYPE_STRING       = 12,
    OPCUA_TYPE_DATETIME     = 13,
    OPCUA_TYPE_GUID         = 14,
    OPCUA_TYPE_BYTESTRING   = 15,
    OPCUA_TYPE_NODEID       = 17,
    OPCUA_TYPE_STATUSCODE   = 19,
    OPCUA_TYPE_QUALIFIEDNAME = 20,
    OPCUA_TYPE_LOCALIZEDTEXT = 21,
    OPCUA_TYPE_VARIANT      = 24
} opcua_type_t;

/* OPC UA Node ID Types */
typedef enum {
    OPCUA_NODEID_NUMERIC    = 0,
    OPCUA_NODEID_STRING     = 3,
    OPCUA_NODEID_GUID       = 4,
    OPCUA_NODEID_BYTESTRING = 5
} opcua_nodeid_type_t;

/* OPC UA Node Classes */
typedef enum {
    OPCUA_NC_OBJECT         = 1,
    OPCUA_NC_VARIABLE       = 2,
    OPCUA_NC_METHOD         = 4,
    OPCUA_NC_OBJECTTYPE     = 8,
    OPCUA_NC_VARIABLETYPE   = 16,
    OPCUA_NC_REFERENCETYPE  = 32,
    OPCUA_NC_DATATYPE       = 64,
    OPCUA_NC_VIEW           = 128
} opcua_node_class_t;

/* OPC UA Attribute IDs */
typedef enum {
    OPCUA_ATTR_NODEID       = 1,
    OPCUA_ATTR_NODECLASS    = 2,
    OPCUA_ATTR_BROWSENAME   = 3,
    OPCUA_ATTR_DISPLAYNAME  = 4,
    OPCUA_ATTR_DESCRIPTION  = 5,
    OPCUA_ATTR_VALUE        = 13,
    OPCUA_ATTR_DATATYPE     = 14,
    OPCUA_ATTR_ACCESSLEVEL  = 17
} opcua_attribute_t;

/* OPC UA Message Header */
typedef struct PACKED {
    char        type[3];
    char        is_final;
    uint32_t    size;
} opcua_msg_hdr_t;

/* OPC UA Secure Channel Header */
typedef struct PACKED {
    uint32_t    channel_id;
} opcua_secure_hdr_t;

/* OPC UA Sequence Header */
typedef struct PACKED {
    uint32_t    sequence_num;
    uint32_t    request_id;
} opcua_seq_hdr_t;

/* Node ID */
typedef struct {
    uint16_t    ns;             /* Namespace index */
    opcua_nodeid_type_t type;
    union {
        uint32_t    numeric;
        struct {
            char    *data;
            uint16_t len;
        } string;
        uint8_t     guid[16];
    } id;
} opcua_nodeid_t;

/* Variant */
typedef struct {
    opcua_type_t type;
    union {
        bool        boolean;
        int8_t      sbyte;
        uint8_t     byte;
        int16_t     i16;
        uint16_t    u16;
        int32_t     i32;
        uint32_t    u32;
        int64_t     i64;
        uint64_t    u64;
        float       f32;
        double      f64;
        struct {
            char    *data;
            uint32_t len;
        } string;
        opcua_nodeid_t nodeid;
    } value;
} opcua_variant_t;

/* Data Value */
typedef struct {
    opcua_variant_t value;
    uint32_t        status;
    uint64_t        source_time;
    uint64_t        server_time;
} opcua_datavalue_t;

/* Node */
typedef struct opcua_node {
    opcua_nodeid_t      node_id;
    opcua_node_class_t  node_class;
    char                *browse_name;
    char                *display_name;
    opcua_variant_t     value;
    opcua_type_t        data_type;
    uint8_t             access_level;

    struct opcua_node   *parent;
    struct opcua_node   *children;
    struct opcua_node   *next;
} opcua_node_t;

/* Session */
typedef struct {
    uint32_t        session_id;
    uint32_t        auth_token;
    bool            activated;
    tick_t          timeout;
    tick_t          last_activity;
    uint32_t        channel_id;
} opcua_session_t;

/* Subscription */
typedef struct opcua_subscription {
    uint32_t        subscription_id;
    uint32_t        session_id;
    double          publishing_interval;
    uint32_t        max_notifications;
    bool            enabled;

    /* Monitored Items */
    struct {
        opcua_nodeid_t  node_id;
        uint32_t        attribute_id;
        uint32_t        client_handle;
        double          sampling_interval;
        bool            active;
    } items[32];
    uint32_t        item_count;

    struct opcua_subscription *next;
} opcua_subscription_t;

/* Server Context */
typedef struct {
    /* Network */
    int             socket;
    uint16_t        port;
    bool            running;

    /* Secure Channel */
    uint32_t        channel_id;
    uint32_t        sequence_num;
    uint32_t        request_id;

    /* Sessions */
    opcua_session_t sessions[CONFIG_OPCUA_MAX_SESSIONS];
    uint32_t        session_count;

    /* Subscriptions */
    opcua_subscription_t *subscriptions;

    /* Address Space */
    opcua_node_t    *root_node;
    opcua_node_t    *nodes;
    uint32_t        node_count;

    /* Callbacks */
    status_t        (*on_read)(opcua_nodeid_t *node_id, opcua_variant_t *value);
    status_t        (*on_write)(opcua_nodeid_t *node_id, opcua_variant_t *value);

    spinlock_t      lock;
} opcua_server_t;

/* Client Context */
typedef struct {
    int             socket;
    sockaddr_t      server_addr;
    uint32_t        channel_id;
    uint32_t        session_id;
    uint32_t        auth_token;
    uint32_t        sequence_num;
    uint32_t        request_id;
    tick_t          timeout;
} opcua_client_t;

/* Server API */
status_t opcua_server_init(opcua_server_t *server);
status_t opcua_server_start(opcua_server_t *server, uint16_t port);
void opcua_server_stop(opcua_server_t *server);
void opcua_server_poll(opcua_server_t *server);

/* Address Space */
opcua_node_t *opcua_add_node(opcua_server_t *server, opcua_node_t *parent,
                              opcua_nodeid_t *node_id, opcua_node_class_t nc,
                              const char *browse_name, const char *display_name);
opcua_node_t *opcua_find_node(opcua_server_t *server, opcua_nodeid_t *node_id);
status_t opcua_set_value(opcua_node_t *node, opcua_variant_t *value);
status_t opcua_get_value(opcua_node_t *node, opcua_variant_t *value);

/* Client API */
status_t opcua_client_init(opcua_client_t *client);
status_t opcua_client_connect(opcua_client_t *client, uint32_t ip, uint16_t port);
status_t opcua_client_create_session(opcua_client_t *client);
status_t opcua_client_activate_session(opcua_client_t *client);
void opcua_client_disconnect(opcua_client_t *client);

/* Read/Write Operations (Zero-Copy) */
status_t opcua_client_read(opcua_client_t *client, opcua_nodeid_t *node_id,
                           uint32_t attribute, opcua_datavalue_t *result);
status_t opcua_client_write(opcua_client_t *client, opcua_nodeid_t *node_id,
                            uint32_t attribute, opcua_variant_t *value);
status_t opcua_client_browse(opcua_client_t *client, opcua_nodeid_t *node_id);

/* Encoding/Decoding */
uint8_t *opcua_encode_nodeid(uint8_t *buf, opcua_nodeid_t *node_id);
uint8_t *opcua_decode_nodeid(uint8_t *buf, opcua_nodeid_t *node_id);
uint8_t *opcua_encode_variant(uint8_t *buf, opcua_variant_t *variant);
uint8_t *opcua_decode_variant(uint8_t *buf, opcua_variant_t *variant);
uint8_t *opcua_encode_string(uint8_t *buf, const char *str);
uint8_t *opcua_decode_string(uint8_t *buf, char **str, uint32_t *len);

#endif /* OPCUA_H */
