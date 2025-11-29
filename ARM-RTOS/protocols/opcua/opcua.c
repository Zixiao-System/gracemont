/*
 * Gracemont Industrial Control Framework
 * Zero-Copy OPC UA Implementation
 */

#include "opcua.h"
#include "rtos_config.h"

/*
 * Binary Encoding Helpers
 */
static inline void write_u8(uint8_t **buf, uint8_t val)
{
    *(*buf)++ = val;
}

static inline void write_u16(uint8_t **buf, uint16_t val)
{
    *(*buf)++ = val & 0xFF;
    *(*buf)++ = (val >> 8) & 0xFF;
}

static inline void write_u32(uint8_t **buf, uint32_t val)
{
    *(*buf)++ = val & 0xFF;
    *(*buf)++ = (val >> 8) & 0xFF;
    *(*buf)++ = (val >> 16) & 0xFF;
    *(*buf)++ = (val >> 24) & 0xFF;
}

static inline void write_u64(uint8_t **buf, uint64_t val)
{
    write_u32(buf, val & 0xFFFFFFFF);
    write_u32(buf, (val >> 32) & 0xFFFFFFFF);
}

static inline uint8_t read_u8(uint8_t **buf)
{
    return *(*buf)++;
}

static inline uint16_t read_u16(uint8_t **buf)
{
    uint16_t val = (*buf)[0] | ((*buf)[1] << 8);
    *buf += 2;
    return val;
}

static inline uint32_t read_u32(uint8_t **buf)
{
    uint32_t val = (*buf)[0] | ((*buf)[1] << 8) |
                   ((*buf)[2] << 16) | ((*buf)[3] << 24);
    *buf += 4;
    return val;
}

static inline uint64_t read_u64(uint8_t **buf)
{
    uint64_t low = read_u32(buf);
    uint64_t high = read_u32(buf);
    return low | (high << 32);
}

/*
 * String Encoding
 */
uint8_t *opcua_encode_string(uint8_t *buf, const char *str)
{
    if (str == NULL) {
        write_u32(&buf, 0xFFFFFFFF);  /* Null string */
        return buf;
    }

    uint32_t len = 0;
    const char *p = str;
    while (*p++) len++;

    write_u32(&buf, len);
    for (uint32_t i = 0; i < len; i++) {
        *buf++ = str[i];
    }
    return buf;
}

uint8_t *opcua_decode_string(uint8_t *buf, char **str, uint32_t *len)
{
    uint32_t slen = read_u32(&buf);
    if (slen == 0xFFFFFFFF) {
        *str = NULL;
        *len = 0;
        return buf;
    }

    *str = (char *)buf;
    *len = slen;
    return buf + slen;
}

/*
 * NodeId Encoding
 */
uint8_t *opcua_encode_nodeid(uint8_t *buf, opcua_nodeid_t *node_id)
{
    if (node_id->type == OPCUA_NODEID_NUMERIC) {
        if (node_id->ns == 0 && node_id->id.numeric < 256) {
            /* Two-byte encoding */
            write_u8(&buf, 0x00);
            write_u8(&buf, node_id->id.numeric);
        } else if (node_id->ns < 256 && node_id->id.numeric < 65536) {
            /* Four-byte encoding */
            write_u8(&buf, 0x01);
            write_u8(&buf, node_id->ns);
            write_u16(&buf, node_id->id.numeric);
        } else {
            /* Numeric encoding */
            write_u8(&buf, 0x02);
            write_u16(&buf, node_id->ns);
            write_u32(&buf, node_id->id.numeric);
        }
    } else if (node_id->type == OPCUA_NODEID_STRING) {
        write_u8(&buf, 0x03);
        write_u16(&buf, node_id->ns);
        write_u32(&buf, node_id->id.string.len);
        for (uint16_t i = 0; i < node_id->id.string.len; i++) {
            *buf++ = node_id->id.string.data[i];
        }
    }
    return buf;
}

uint8_t *opcua_decode_nodeid(uint8_t *buf, opcua_nodeid_t *node_id)
{
    uint8_t encoding = read_u8(&buf);

    switch (encoding & 0x0F) {
    case 0x00:  /* Two-byte */
        node_id->type = OPCUA_NODEID_NUMERIC;
        node_id->ns = 0;
        node_id->id.numeric = read_u8(&buf);
        break;
    case 0x01:  /* Four-byte */
        node_id->type = OPCUA_NODEID_NUMERIC;
        node_id->ns = read_u8(&buf);
        node_id->id.numeric = read_u16(&buf);
        break;
    case 0x02:  /* Numeric */
        node_id->type = OPCUA_NODEID_NUMERIC;
        node_id->ns = read_u16(&buf);
        node_id->id.numeric = read_u32(&buf);
        break;
    case 0x03:  /* String */
        node_id->type = OPCUA_NODEID_STRING;
        node_id->ns = read_u16(&buf);
        node_id->id.string.len = read_u32(&buf);
        node_id->id.string.data = (char *)buf;
        buf += node_id->id.string.len;
        break;
    }
    return buf;
}

/*
 * Variant Encoding
 */
uint8_t *opcua_encode_variant(uint8_t *buf, opcua_variant_t *variant)
{
    write_u8(&buf, variant->type);

    switch (variant->type) {
    case OPCUA_TYPE_BOOLEAN:
        write_u8(&buf, variant->value.boolean ? 1 : 0);
        break;
    case OPCUA_TYPE_SBYTE:
        write_u8(&buf, variant->value.sbyte);
        break;
    case OPCUA_TYPE_BYTE:
        write_u8(&buf, variant->value.byte);
        break;
    case OPCUA_TYPE_INT16:
        write_u16(&buf, variant->value.i16);
        break;
    case OPCUA_TYPE_UINT16:
        write_u16(&buf, variant->value.u16);
        break;
    case OPCUA_TYPE_INT32:
        write_u32(&buf, variant->value.i32);
        break;
    case OPCUA_TYPE_UINT32:
        write_u32(&buf, variant->value.u32);
        break;
    case OPCUA_TYPE_INT64:
        write_u64(&buf, variant->value.i64);
        break;
    case OPCUA_TYPE_UINT64:
        write_u64(&buf, variant->value.u64);
        break;
    case OPCUA_TYPE_FLOAT:
        write_u32(&buf, *(uint32_t *)&variant->value.f32);
        break;
    case OPCUA_TYPE_DOUBLE:
        write_u64(&buf, *(uint64_t *)&variant->value.f64);
        break;
    case OPCUA_TYPE_STRING:
        buf = opcua_encode_string(buf, variant->value.string.data);
        break;
    case OPCUA_TYPE_NODEID:
        buf = opcua_encode_nodeid(buf, &variant->value.nodeid);
        break;
    default:
        break;
    }
    return buf;
}

uint8_t *opcua_decode_variant(uint8_t *buf, opcua_variant_t *variant)
{
    variant->type = read_u8(&buf);

    switch (variant->type) {
    case OPCUA_TYPE_BOOLEAN:
        variant->value.boolean = read_u8(&buf) != 0;
        break;
    case OPCUA_TYPE_SBYTE:
        variant->value.sbyte = read_u8(&buf);
        break;
    case OPCUA_TYPE_BYTE:
        variant->value.byte = read_u8(&buf);
        break;
    case OPCUA_TYPE_INT16:
        variant->value.i16 = read_u16(&buf);
        break;
    case OPCUA_TYPE_UINT16:
        variant->value.u16 = read_u16(&buf);
        break;
    case OPCUA_TYPE_INT32:
        variant->value.i32 = read_u32(&buf);
        break;
    case OPCUA_TYPE_UINT32:
        variant->value.u32 = read_u32(&buf);
        break;
    case OPCUA_TYPE_INT64:
        variant->value.i64 = read_u64(&buf);
        break;
    case OPCUA_TYPE_UINT64:
        variant->value.u64 = read_u64(&buf);
        break;
    case OPCUA_TYPE_FLOAT: {
        uint32_t tmp = read_u32(&buf);
        variant->value.f32 = *(float *)&tmp;
        break;
    }
    case OPCUA_TYPE_DOUBLE: {
        uint64_t tmp = read_u64(&buf);
        variant->value.f64 = *(double *)&tmp;
        break;
    }
    case OPCUA_TYPE_STRING:
        buf = opcua_decode_string(buf, &variant->value.string.data,
                                  &variant->value.string.len);
        break;
    case OPCUA_TYPE_NODEID:
        buf = opcua_decode_nodeid(buf, &variant->value.nodeid);
        break;
    default:
        break;
    }
    return buf;
}

/*
 * Server Initialization
 */
status_t opcua_server_init(opcua_server_t *server)
{
    server->socket = -1;
    server->port = CONFIG_OPCUA_PORT;
    server->running = false;
    server->channel_id = 1;
    server->sequence_num = 1;
    server->request_id = 0;
    server->session_count = 0;
    server->subscriptions = NULL;
    server->root_node = NULL;
    server->nodes = NULL;
    server->node_count = 0;
    server->on_read = NULL;
    server->on_write = NULL;
    server->lock = (spinlock_t)SPINLOCK_INIT;

    return STATUS_OK;
}

/*
 * Address Space Management
 */
opcua_node_t *opcua_add_node(opcua_server_t *server, opcua_node_t *parent,
                              opcua_nodeid_t *node_id, opcua_node_class_t nc,
                              const char *browse_name, const char *display_name)
{
    opcua_node_t *node = heap_alloc(sizeof(opcua_node_t));
    if (node == NULL) return NULL;

    node->node_id = *node_id;
    node->node_class = nc;
    node->browse_name = (char *)browse_name;
    node->display_name = (char *)display_name;
    node->value.type = OPCUA_TYPE_NULL;
    node->data_type = OPCUA_TYPE_NULL;
    node->access_level = 0x03;  /* Read/Write */
    node->parent = parent;
    node->children = NULL;
    node->next = NULL;

    spin_lock(&server->lock);

    if (parent != NULL) {
        node->next = parent->children;
        parent->children = node;
    } else if (server->root_node == NULL) {
        server->root_node = node;
    }

    server->node_count++;
    spin_unlock(&server->lock);

    return node;
}

opcua_node_t *opcua_find_node(opcua_server_t *server, opcua_nodeid_t *node_id)
{
    opcua_node_t *find_recursive(opcua_node_t *node, opcua_nodeid_t *id) {
        if (node == NULL) return NULL;

        if (node->node_id.ns == id->ns &&
            node->node_id.type == id->type) {
            if (id->type == OPCUA_NODEID_NUMERIC &&
                node->node_id.id.numeric == id->id.numeric) {
                return node;
            }
        }

        opcua_node_t *found = find_recursive(node->children, id);
        if (found) return found;

        return find_recursive(node->next, id);
    }

    spin_lock(&server->lock);
    opcua_node_t *node = find_recursive(server->root_node, node_id);
    spin_unlock(&server->lock);

    return node;
}

status_t opcua_set_value(opcua_node_t *node, opcua_variant_t *value)
{
    if (node == NULL) return STATUS_INVALID;
    node->value = *value;
    return STATUS_OK;
}

status_t opcua_get_value(opcua_node_t *node, opcua_variant_t *value)
{
    if (node == NULL) return STATUS_INVALID;
    *value = node->value;
    return STATUS_OK;
}

/*
 * Build OPC UA Message (Zero-Copy)
 */
static zbuf_t *opcua_build_message(opcua_server_t *server, const char *type,
                                    uint8_t *payload, uint32_t payload_len)
{
    uint32_t msg_len = 8 + payload_len;  /* Header + payload */

    zbuf_t *zb = zbuf_alloc_tx(msg_len);
    if (zb == NULL) return NULL;

    uint8_t *buf = zbuf_put(zb, msg_len);

    /* Message header */
    buf[0] = type[0];
    buf[1] = type[1];
    buf[2] = type[2];
    buf[3] = 'F';  /* Final */
    buf[4] = msg_len & 0xFF;
    buf[5] = (msg_len >> 8) & 0xFF;
    buf[6] = (msg_len >> 16) & 0xFF;
    buf[7] = (msg_len >> 24) & 0xFF;

    /* Copy payload */
    for (uint32_t i = 0; i < payload_len; i++) {
        buf[8 + i] = payload[i];
    }

    return zb;
}

/*
 * Process Hello Message
 */
static zbuf_t *opcua_handle_hello(opcua_server_t *server, zbuf_t *req)
{
    uint8_t ack[28];
    uint8_t *p = ack;

    write_u32(&p, 0);           /* Protocol version */
    write_u32(&p, 65536);       /* Receive buffer size */
    write_u32(&p, 65536);       /* Send buffer size */
    write_u32(&p, 0);           /* Max message size (0 = no limit) */
    write_u32(&p, 0);           /* Max chunk count */

    return opcua_build_message(server, "ACK", ack, 20);
}

/*
 * Process OpenSecureChannel
 */
static zbuf_t *opcua_handle_open_channel(opcua_server_t *server, zbuf_t *req)
{
    uint8_t *data = req->data + 12;  /* Skip header + security header */

    /* Parse request */
    uint32_t req_type = read_u32(&data);
    uint32_t sec_mode = read_u32(&data);

    /* Build response */
    uint8_t resp[128];
    uint8_t *p = resp;

    /* Security header */
    write_u32(&p, server->channel_id);

    /* Asymmetric security header (None security) */
    write_u32(&p, 47);  /* Security policy URI length */
    const char *policy = "http://opcfoundation.org/UA/SecurityPolicy#None";
    for (int i = 0; i < 47; i++) *p++ = policy[i];
    write_u32(&p, 0xFFFFFFFF);  /* Sender certificate (null) */
    write_u32(&p, 0xFFFFFFFF);  /* Receiver thumbprint (null) */

    /* Sequence header */
    write_u32(&p, server->sequence_num++);
    write_u32(&p, server->request_id++);

    /* Response body */
    /* Type ID for OpenSecureChannelResponse */
    write_u8(&p, 0x01);
    write_u8(&p, 0x00);
    write_u16(&p, 449);  /* Type ID */

    /* Response header */
    write_u64(&p, get_system_ticks() * 10000);  /* Timestamp */
    write_u32(&p, 0);   /* Request handle */
    write_u32(&p, OPCUA_STATUS_GOOD);
    write_u8(&p, 0);    /* Service diagnostics */
    write_u32(&p, 0xFFFFFFFF);  /* String table (null) */
    write_u32(&p, 0);   /* Additional header */

    /* Channel parameters */
    write_u32(&p, 0);   /* Server protocol version */
    write_u32(&p, server->channel_id);
    write_u32(&p, 1);   /* Token ID */
    write_u64(&p, get_system_ticks() * 10000);  /* Created at */
    write_u32(&p, 3600000);  /* Revised lifetime (1 hour) */
    write_u32(&p, 0xFFFFFFFF);  /* Server nonce (null) */

    return opcua_build_message(server, "OPN", resp, p - resp);
}

/*
 * Process Service Request
 */
static zbuf_t *opcua_handle_service(opcua_server_t *server, zbuf_t *req)
{
    uint8_t *data = req->data + 8;  /* Skip message header */

    /* Security header */
    uint32_t channel_id = read_u32(&data);
    uint32_t token_id = read_u32(&data);

    /* Sequence header */
    uint32_t seq_num = read_u32(&data);
    uint32_t req_id = read_u32(&data);

    /* Type ID */
    uint8_t encoding = read_u8(&data);
    uint8_t ns = read_u8(&data);
    uint16_t type_id = read_u16(&data);

    /* Prepare response buffer */
    uint8_t resp[1024];
    uint8_t *p = resp;

    /* Security header */
    write_u32(&p, channel_id);
    write_u32(&p, token_id);

    /* Sequence header */
    write_u32(&p, server->sequence_num++);
    write_u32(&p, req_id);

    /* Handle service */
    switch (type_id) {
    case 428: {  /* GetEndpointsRequest */
        /* Type ID for GetEndpointsResponse */
        write_u8(&p, 0x01);
        write_u8(&p, 0x00);
        write_u16(&p, 431);

        /* Response header */
        write_u64(&p, get_system_ticks() * 10000);
        write_u32(&p, 0);
        write_u32(&p, OPCUA_STATUS_GOOD);
        write_u8(&p, 0);
        write_u32(&p, 0xFFFFFFFF);
        write_u32(&p, 0);

        /* Endpoints array (1 endpoint) */
        write_u32(&p, 1);

        /* Endpoint URL */
        char url[64];
        uint32_t url_len = 0;
        url[url_len++] = 'o'; url[url_len++] = 'p'; url[url_len++] = 'c';
        url[url_len++] = '.'; url[url_len++] = 't'; url[url_len++] = 'c';
        url[url_len++] = 'p'; url[url_len++] = ':'; url[url_len++] = '/';
        url[url_len++] = '/'; url[url_len++] = 'l'; url[url_len++] = 'o';
        url[url_len++] = 'c'; url[url_len++] = 'a'; url[url_len++] = 'l';
        url[url_len++] = 'h'; url[url_len++] = 'o'; url[url_len++] = 's';
        url[url_len++] = 't'; url[url_len++] = ':'; url[url_len++] = '4';
        url[url_len++] = '8'; url[url_len++] = '4'; url[url_len++] = '0';
        write_u32(&p, url_len);
        for (uint32_t i = 0; i < url_len; i++) *p++ = url[i];

        /* Server (simplified) */
        write_u32(&p, 0xFFFFFFFF);  /* Application URI */
        write_u32(&p, 0xFFFFFFFF);  /* Product URI */
        write_u8(&p, 0x02);         /* Localized text encoding */
        write_u32(&p, 0xFFFFFFFF);  /* Locale */
        write_u32(&p, 14);          /* Text length */
        const char *name = "RTOS OPC UA";
        for (int i = 0; i < 14; i++) *p++ = name[i];
        write_u32(&p, 0);           /* Application type */
        write_u32(&p, 0xFFFFFFFF);  /* Gateway server URI */
        write_u32(&p, 0xFFFFFFFF);  /* Discovery profile URI */
        write_u32(&p, 0);           /* Discovery URLs */

        /* Server certificate */
        write_u32(&p, 0xFFFFFFFF);

        /* Security mode */
        write_u32(&p, 1);  /* None */

        /* Security policy */
        write_u32(&p, 47);
        const char *policy = "http://opcfoundation.org/UA/SecurityPolicy#None";
        for (int i = 0; i < 47; i++) *p++ = policy[i];

        /* User identity tokens */
        write_u32(&p, 1);
        write_u32(&p, 9);           /* Policy ID */
        const char *pid = "anonymous";
        for (int i = 0; i < 9; i++) *p++ = pid[i];
        write_u32(&p, 0);           /* Token type (anonymous) */
        write_u32(&p, 0xFFFFFFFF);  /* Issued token type */
        write_u32(&p, 0xFFFFFFFF);  /* Issuer endpoint URL */
        write_u32(&p, 0xFFFFFFFF);  /* Security policy URI */

        /* Transport profile */
        write_u32(&p, 65);
        const char *transport = "http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary";
        for (int i = 0; i < 65; i++) *p++ = transport[i];

        write_u8(&p, 0);  /* Security level */
        break;
    }

    case 461: {  /* CreateSessionRequest */
        /* Type ID for CreateSessionResponse */
        write_u8(&p, 0x01);
        write_u8(&p, 0x00);
        write_u16(&p, 464);

        /* Response header */
        write_u64(&p, get_system_ticks() * 10000);
        write_u32(&p, 0);
        write_u32(&p, OPCUA_STATUS_GOOD);
        write_u8(&p, 0);
        write_u32(&p, 0xFFFFFFFF);
        write_u32(&p, 0);

        /* Session ID */
        uint32_t session_id = server->session_count + 1;
        write_u8(&p, 0x02);
        write_u16(&p, 1);
        write_u32(&p, session_id);

        /* Authentication token */
        write_u8(&p, 0x02);
        write_u16(&p, 1);
        write_u32(&p, session_id);

        write_u64(&p, 0);                   /* Revised session timeout */
        write_u32(&p, 0xFFFFFFFF);          /* Server nonce */
        write_u32(&p, 0xFFFFFFFF);          /* Server certificate */
        write_u32(&p, 0);                   /* Server endpoints */
        write_u32(&p, 0);                   /* Server software certificates */
        write_u32(&p, 0xFFFFFFFF);          /* Server signature */
        write_u32(&p, 0);                   /* Max request message size */

        /* Create session */
        if (server->session_count < CONFIG_OPCUA_MAX_SESSIONS) {
            opcua_session_t *session = &server->sessions[server->session_count++];
            session->session_id = session_id;
            session->auth_token = session_id;
            session->activated = false;
            session->channel_id = channel_id;
            session->last_activity = get_system_ticks();
        }
        break;
    }

    case 467: {  /* ActivateSessionRequest */
        /* Type ID for ActivateSessionResponse */
        write_u8(&p, 0x01);
        write_u8(&p, 0x00);
        write_u16(&p, 470);

        /* Response header */
        write_u64(&p, get_system_ticks() * 10000);
        write_u32(&p, 0);
        write_u32(&p, OPCUA_STATUS_GOOD);
        write_u8(&p, 0);
        write_u32(&p, 0xFFFFFFFF);
        write_u32(&p, 0);

        write_u32(&p, 0xFFFFFFFF);  /* Server nonce */
        write_u32(&p, 0);           /* Results */
        write_u32(&p, 0);           /* Diagnostic infos */
        break;
    }

    case 631: {  /* ReadRequest */
        /* Skip request header */
        data += 24;

        /* Read parameters */
        uint64_t max_age = read_u64(&data);
        uint32_t timestamps = read_u32(&data);
        uint32_t node_count = read_u32(&data);

        /* Type ID for ReadResponse */
        write_u8(&p, 0x01);
        write_u8(&p, 0x00);
        write_u16(&p, 634);

        /* Response header */
        write_u64(&p, get_system_ticks() * 10000);
        write_u32(&p, 0);
        write_u32(&p, OPCUA_STATUS_GOOD);
        write_u8(&p, 0);
        write_u32(&p, 0xFFFFFFFF);
        write_u32(&p, 0);

        /* Results array */
        write_u32(&p, node_count);

        for (uint32_t i = 0; i < node_count && i < 16; i++) {
            opcua_nodeid_t node_id;
            data = opcua_decode_nodeid(data, &node_id);
            uint32_t attr_id = read_u32(&data);

            /* Skip index range and data encoding */
            uint32_t ir_len = read_u32(&data);
            if (ir_len != 0xFFFFFFFF) data += ir_len;
            data += 3;

            /* Find node and get value */
            opcua_node_t *node = opcua_find_node(server, &node_id);
            if (node != NULL) {
                if (server->on_read) {
                    server->on_read(&node_id, &node->value);
                }

                /* DataValue */
                write_u8(&p, 0x01);  /* Has value */
                p = opcua_encode_variant(p, &node->value);
            } else {
                /* Bad status */
                write_u8(&p, 0x02);  /* Has status */
                write_u32(&p, OPCUA_STATUS_BAD_NODEID_UNKNOWN);
            }
        }

        write_u32(&p, 0);  /* Diagnostic infos */
        break;
    }

    case 673: {  /* WriteRequest */
        /* Skip request header */
        data += 24;

        uint32_t node_count = read_u32(&data);

        /* Type ID for WriteResponse */
        write_u8(&p, 0x01);
        write_u8(&p, 0x00);
        write_u16(&p, 676);

        /* Response header */
        write_u64(&p, get_system_ticks() * 10000);
        write_u32(&p, 0);
        write_u32(&p, OPCUA_STATUS_GOOD);
        write_u8(&p, 0);
        write_u32(&p, 0xFFFFFFFF);
        write_u32(&p, 0);

        /* Results array */
        write_u32(&p, node_count);

        for (uint32_t i = 0; i < node_count && i < 16; i++) {
            opcua_nodeid_t node_id;
            data = opcua_decode_nodeid(data, &node_id);
            uint32_t attr_id = read_u32(&data);

            /* Skip index range */
            uint32_t ir_len = read_u32(&data);
            if (ir_len != 0xFFFFFFFF) data += ir_len;

            /* Read value */
            uint8_t encoding = read_u8(&data);
            opcua_variant_t value;
            if (encoding & 0x01) {
                data = opcua_decode_variant(data, &value);
            }

            /* Find node and set value */
            opcua_node_t *node = opcua_find_node(server, &node_id);
            if (node != NULL) {
                node->value = value;
                if (server->on_write) {
                    server->on_write(&node_id, &value);
                }
                write_u32(&p, OPCUA_STATUS_GOOD);
            } else {
                write_u32(&p, OPCUA_STATUS_BAD_NODEID_UNKNOWN);
            }
        }

        write_u32(&p, 0);  /* Diagnostic infos */
        break;
    }

    default:
        /* Service fault */
        write_u8(&p, 0x01);
        write_u8(&p, 0x00);
        write_u16(&p, 397);  /* ServiceFault */

        write_u64(&p, get_system_ticks() * 10000);
        write_u32(&p, 0);
        write_u32(&p, OPCUA_STATUS_BAD);
        write_u8(&p, 0);
        write_u32(&p, 0xFFFFFFFF);
        write_u32(&p, 0);
        break;
    }

    return opcua_build_message(server, "MSG", resp, p - resp);
}

/*
 * Server Start
 */
status_t opcua_server_start(opcua_server_t *server, uint16_t port)
{
    server->socket = sock_socket(SOCK_STREAM);
    if (server->socket < 0) return STATUS_ERROR;

    sockaddr_t addr = { .addr = IP4_ADDR_ANY, .port = port };
    if (sock_bind(server->socket, &addr) < 0) {
        sock_close(server->socket);
        return STATUS_ERROR;
    }

    if (sock_listen(server->socket, 5) < 0) {
        sock_close(server->socket);
        return STATUS_ERROR;
    }

    server->port = port;
    server->running = true;
    return STATUS_OK;
}

/*
 * Server Poll
 */
void opcua_server_poll(opcua_server_t *server)
{
    if (!server->running) return;

    sockaddr_t client_addr;
    int client_fd = sock_accept(server->socket, &client_addr);
    if (client_fd < 0) return;

    while (1) {
        zbuf_t *req = sock_recv_zbuf(client_fd);
        if (req == NULL || req->len < 8) {
            if (req) zbuf_free(req);
            break;
        }

        /* Parse message type */
        char type[4] = { req->data[0], req->data[1], req->data[2], 0 };
        zbuf_t *resp = NULL;

        if (type[0] == 'H' && type[1] == 'E' && type[2] == 'L') {
            resp = opcua_handle_hello(server, req);
        } else if (type[0] == 'O' && type[1] == 'P' && type[2] == 'N') {
            resp = opcua_handle_open_channel(server, req);
        } else if (type[0] == 'M' && type[1] == 'S' && type[2] == 'G') {
            resp = opcua_handle_service(server, req);
        } else if (type[0] == 'C' && type[1] == 'L' && type[2] == 'O') {
            zbuf_free(req);
            break;
        }

        zbuf_free(req);

        if (resp != NULL) {
            sock_send_zbuf(client_fd, resp);
        }
    }

    sock_close(client_fd);
}

void opcua_server_stop(opcua_server_t *server)
{
    server->running = false;
    if (server->socket >= 0) {
        sock_close(server->socket);
        server->socket = -1;
    }
}
