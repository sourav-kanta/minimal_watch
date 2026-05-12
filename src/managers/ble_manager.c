#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/addr.h>
#include "managers/ble_manager.h"
#include "managers/event_manager.h"
#include "ble_types.h"


LOG_MODULE_REGISTER(BLE, LOG_LEVEL_INF);

/* @TODO 
 * Force that 6-digit random passkey for bonding.
 * Discovery & Connection: Device starts advertising. Once connected, they establish a permanent bond so the passkey isn't needed again.
 * Timing Parameters:
 * Interval: 1 second ().
 * Latency: 0 (ensures both wake up every second).
 * Burst: 150 ms (Link Layer event length).
 * Set dbm to 0
 * Supervision timeout =4s
 * Throughput Tuning:
 * MTU: Set to at least 247 bytes to fit as many packets as possible within 244 bytes.
 * Speed: Manually trigger the 2 Mbps PHY update after connection to minimize radio airtime.
 * Bidirectional Logic:
 * Phone
 *  Device: Phone uses Write w/o response. ESP32 processes the data in the 850 ms idle window.
 * Device
 *  Phone: ESP32 uses Notifications. Phone "subscribes" once via the CCC descriptor. Only then can the ESP notify, otherwise error => so push to queue till subscribed
 * The "Sync" Cycle: Each device fills its outgoing buffer (Queue) during the 850 ms computation period, ready to blast everything in the next 1-second window.
*/

static struct bt_uuid_128 mw_service_uuid = BT_UUID_INIT_128(BT_UUID_SMARTWATCH_SERVICE_VAL);
static struct bt_uuid_128 rx_chrc_uuid = BT_UUID_INIT_128(BT_UUID_RX_CHRC_VAL);
static struct bt_uuid_128 tx_chrc_uuid = BT_UUID_INIT_128(BT_UUID_TX_CHRC_VAL);

const uint32_t BLE_ADV_INTERVAL_MAX = 0x064;
const uint32_t BLE_ADV_INTERVAL_MIN = 0x064;

static const uint16_t BLE_CONN_INTERVAL_MIN = 0x0320; // 800*1.25 = 1s
static const uint16_t BLE_CONN_INTERVAL_MAX = 0x0348; // 840*1.25 = 1.050s
static const uint16_t BLE_CONN_LATENCY = 0x0000;
static const uint16_t BLE_CONN_TIMEOUT = 0x0190; // 400*10 = 4s

static struct bt_conn *ble_conn = NULL;
static bool ble_connected = false;
static bool notifications_enabled = false;

static struct bt_le_adv_param adv_param_factory = {
    .id = BT_ID_DEFAULT,
    .options = (BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_IDENTITY),
    .interval_min = BLE_ADV_INTERVAL_MIN, // 1 sec
    .interval_max = BLE_ADV_INTERVAL_MAX, // 1 sec 
};

static struct bt_le_conn_param conn_params = {
    .interval_min = BLE_CONN_INTERVAL_MIN,
    .interval_max = BLE_CONN_INTERVAL_MAX,
    .latency = BLE_CONN_LATENCY,
    .timeout = BLE_CONN_TIMEOUT
};

static struct bt_le_adv_param adv_param_filtered;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static bt_addr_le_t bonded_addr;
static uint16_t effective_mtu=0;

/**
 * @brief This is the peripheral TX buffer, so the device needs to write to it
 * to send commands and responses to phone
 */
static uint8_t tx_buffer[517];
static ble_msg_t tx_cur_msg;
static uint16_t tx_offset = 0;
static uint8_t tx_msg_active = 0;


/**
 * @brief Circular buffer to store pending messages till next connection window
 */
static ble_msg_t ble_tx_fifo[BLE_FIFO_SIZE];
/**
 * @brief Index of TX next write position
 */
static uint16_t tx_write_idx = 0;
/**
 * @brief Index of TX read position
 */
static uint16_t tx_read_idx = 0;
/**
 * @brief Spin lock to make TX FIFO thread safe 
 */
static struct k_spinlock tx_fifo_lock;


/**
 * @brief FIFO to strore produced messages from BLE RX
 */
static ble_msg_t ble_rx_fifo[BLE_FIFO_SIZE];
/**
 * @brief Index of RX next write position
 */
static uint16_t rx_write_idx = 0;
/**
 * @brief Index of RX read position
 */
static uint16_t rx_read_idx = 0;
/**
 * @brief Spin lock to make RX FIFO thread safe 
 */
static struct k_spinlock rx_fifo_lock;

typedef struct {
    rx_state_t state;
    ble_msg_t cur_msg;
    uint16_t offset;
    uint8_t msg_active;
} rx_ctx_t;

static rx_ctx_t rx_ctx;


/**
 * @brief Check if FIFO push is safe 
 *
 * @return 
 */
static inline bool fifo_is_full_unsafe(uint16_t write_idx, uint16_t read_idx)
{
    return ((write_idx + 1) % BLE_FIFO_SIZE) == read_idx;
}

/**
 * @brief Check if FIFO pop is safe 
 *
 * @return 
 */
static inline bool fifo_is_empty_unsafe(uint16_t write_idx, uint16_t read_idx)
{
    return write_idx == read_idx;
}


/**
 * @brief Adds a message to the FIFO msg buffer, rejects the add if 
 * struct provided is null or payload len exceeds max size 
 *
 * @param msg Pointer of msg to be added
 *
 * @return True on sucess and false otherwise
 */
bool add_ble_msg_to_queue(const ble_msg_t *msg, const ble_comm_type_t type)
{
    struct k_spinlock *fifo_lock;
    ble_msg_t *ble_fifo;
    uint16_t *read_idx, *write_idx;

    if(type == BLE_TX) {
        fifo_lock = &tx_fifo_lock;
        ble_fifo = ble_tx_fifo;
        read_idx = &tx_read_idx;
        write_idx = &tx_write_idx;
    }
    else if(type == BLE_RX) {
        fifo_lock = &rx_fifo_lock;
        ble_fifo = ble_rx_fifo;
        read_idx = &rx_read_idx;
        write_idx = &rx_write_idx;
    }
    else {
        LOG_ERR("Invalid BLE communication type");
        return false;
    }
    
    LOG_INF("Adding a new message to FIFO queue ");

    if (!msg) return false;

    if (msg->hdr.len > MAX_BLE_PAYLOAD_SIZE) {
        return false;
    }

    k_spinlock_key_t key = k_spin_lock(fifo_lock);

    if (fifo_is_full_unsafe((*write_idx), (*read_idx))) {
        LOG_INF("BLE FIFO full, discard oldest message");
        (*read_idx) = ((*read_idx) + 1) % BLE_FIFO_SIZE;
    }

    ble_fifo[(*write_idx)] = *msg;

    (*write_idx) = ((*write_idx) + 1) % BLE_FIFO_SIZE;

    k_spin_unlock(fifo_lock, key);
    return true;
}

/**
 * @brief Pops latest messages from FIFO msg buffer, discards the pop
 * if the provided pointer is NULL 
 *
 * @param out_msg Struct to be populated with the popped msg 
 *
 * @return True if pop successful or pointer supplied is NULL, 
 * False if no more messages are present so we can leave rest of packet
 * empty and know end of work in connection window
 */
static bool get_next_ble_msg(ble_msg_t *out_msg, const ble_comm_type_t type)
{
    struct k_spinlock *fifo_lock;
    ble_msg_t *ble_fifo;
    uint16_t *read_idx, *write_idx;

    if(type == BLE_TX) {
        fifo_lock = &tx_fifo_lock;
        ble_fifo = ble_tx_fifo;
        read_idx = &tx_read_idx;
        write_idx = &tx_write_idx;
    }
    else if(type == BLE_RX) {
        fifo_lock = &rx_fifo_lock;
        ble_fifo = ble_rx_fifo;
        read_idx = &rx_read_idx;
        write_idx = &rx_write_idx;
    }
    else {
        LOG_ERR("Invalid BLE communication type");
        return false;
    }

    if (!out_msg) {
        LOG_ERR("NULL struct, discarding pop");
        return true;
    }

    k_spinlock_key_t key = k_spin_lock(fifo_lock);

    if (fifo_is_empty_unsafe((*write_idx), (*read_idx))) {
        k_spin_unlock(fifo_lock, key);
        return false;
    }

    *out_msg = ble_fifo[(*read_idx)];

    (*read_idx) = ((*read_idx) + 1) % BLE_FIFO_SIZE;

    k_spin_unlock(fifo_lock, key);

    return true;
}

/**
 * @brief Builds the next BLE TX packet from the internal message FIFO.
 *
 * This function encodes one MTU-sized BLE packet into the global TX buffer
 * (`tx_buffer`) by consuming messages from the internal FIFO.
 * 
 * @verbatim
 * It performs message fragmentation across packets while preserving message
 * boundaries using a per-message header and a continuation flag at packet level.
 *
 * ---
 *
 * ## Wire Format
 *
 * Each BLE packet generated by this function has the following format:
 *
 * ```
 * [ MSG_CONT ][ MESSAGE STREAM ... ]
 * ```
 *
 * Where:
 *
 * - MSG_CONT (1 byte)
 *      - 1 = continuation of previous message stream
 *      - 0 = new stream start
 *
 * Each message inside the stream has the format:
 *
 * ```
 * [ MAGIC ][ OPCODE ][ REQ_APP ][ LEN ][ PAYLOAD... ]
 * ```
 *
 * Where:
 *
 * - MAGIC   : 1 byte message boundary identifier
 * - OPCODE  : application-defined command opcode
 * - REQ_APP : requesting application ID
 * - LEN     : payload length in bytes
 * - PAYLOAD : variable length data (may be fragmented across packets)
 *
 * ---
 *
 * ## Fragmentation Rules
 *
 * - Only the PAYLOAD field may be fragmented across multiple BLE packets.
 * - Message headers are always sent atomically (never split).
 * - Messages are read sequentially from the FIFO.
 * - If a message does not fully fit in the current MTU, it is continued in
 *   the next packet without repeating the header.
 *
 * --- 
 * @endverbatim
 * 
 * @param out_len 
 *
 * @return Returns true if packet contains data, false if FIFO empty
 */
static bool ble_build_next_packet(uint16_t *out_len)
{
    if (!out_len) return false;

    uint16_t i = 0;
    LOG_INF("Build packet for MTU : %02u", effective_mtu);
    // packet-level continuation flag
    tx_buffer[i++] = tx_msg_active ? 1 : 0;

    while (i < effective_mtu) {

        /* Load next message if needed */
        if (!tx_msg_active) {
            if (!get_next_ble_msg(&tx_cur_msg, BLE_TX)) {
                break; // FIFO empty
            }

            tx_offset = 0;
            tx_msg_active = 1;
        }

        uint16_t remaining_msg = tx_cur_msg.hdr.len - tx_offset;
        uint16_t remaining_pkt = effective_mtu - i;

        /* Ensure header fits when starting a new message */
        if (tx_offset == 0 && remaining_pkt < (1 + BLE_HDR_SIZE)) {
            break;
        }

        /* Write message header once per message */
        if (tx_offset == 0) {

            tx_buffer[i++] = BLE_MAGIC;

            tx_buffer[i++] = tx_cur_msg.hdr.opcode;
            tx_buffer[i++] = tx_cur_msg.hdr.req_app;
            tx_buffer[i++] = tx_cur_msg.hdr.len;
        }

        remaining_pkt = effective_mtu - i;
        uint16_t chunk = (remaining_msg < remaining_pkt)
                            ? remaining_msg
                            : remaining_pkt;

        memcpy(&tx_buffer[i], &tx_cur_msg.payload[tx_offset], chunk);
        i += chunk;
        tx_offset += chunk;

        /* message complete */
        if (tx_offset >= tx_cur_msg.hdr.len) {
            tx_msg_active = 0;
        }

        /* packet full */
        if (i >= effective_mtu) break;
    }

    *out_len = i;
    return (i > 1);
}

void ble_retrieve_packet(const uint8_t *pkt, uint16_t len)
{
    if (!pkt || len == 0) return;

    uint16_t i = 0;

    uint8_t msg_cont = pkt[i++];

    /* =========================================================
     * CONTINUATION HANDLING
     * ========================================================= */
    if (msg_cont == 0) {
        /* New stream → reset parser */
        rx_ctx.state = RX_WAIT_MAGIC;
        rx_ctx.offset = 0;
        rx_ctx.msg_active = 0;
    }
    else {
        /* Continuation expected but no active message → resync */
        if (!rx_ctx.msg_active) {
            LOG_ERR("Unexpected continuation packet → resync");
            rx_ctx.state = RX_WAIT_MAGIC;
            rx_ctx.offset = 0;
        }
    }

    /* =========================================================
     * STREAM PARSER
     * ========================================================= */
    while (i < len) {

        switch (rx_ctx.state) {

        /* ---------------- WAIT FOR MAGIC ---------------- */
        case RX_WAIT_MAGIC:

            if (pkt[i] != BLE_MAGIC) {
                i++; /* Skip garbage */
                break;
            }

            // Handling edge case where we are already in desync and trying to 
            // find next magic, say we get magic in last byte of packet now we 
            // shouldnt consume the magic and set state to header as header
            // will start from next packet first byte (MSG_CONT flag byte) and
            // fail copying garbage in payload (as len is incorrect)
            if (i + 4 > len) {
               /* Not enough bytes for full header → wait next packet */
                LOG_ERR("Protocol violation: partial header detected at packet boundary");
                return;   
            }

            /* Found potential message start */
            rx_ctx.state = RX_READ_HEADER;
            i++;
            break;

        /* ---------------- READ HEADER ---------------- */
        case RX_READ_HEADER:

            if (i + 3 > len) {
                /* Incomplete header → wait next packet */
                return;
            }

            rx_ctx.cur_msg.hdr.opcode  = pkt[i++];
            rx_ctx.cur_msg.hdr.req_app = pkt[i++];
            rx_ctx.cur_msg.hdr.len     = pkt[i++];

            /* Length validation */
            if (rx_ctx.cur_msg.hdr.len > MAX_BLE_PAYLOAD_SIZE) {
                LOG_ERR("Invalid length: %u", rx_ctx.cur_msg.hdr.len);

                /* Resync instead of drop-all */
                rx_ctx.state = RX_WAIT_MAGIC;
                rx_ctx.msg_active = 0;
                break;
            }

            rx_ctx.offset = 0;
            rx_ctx.msg_active = 1;

            /* ----------- HANDLE EMPTY PAYLOAD ----------*/
            if (rx_ctx.cur_msg.hdr.len == 0) {

                add_ble_msg_to_queue(&rx_ctx.cur_msg, BLE_RX);

                rx_ctx.msg_active = 0;
                rx_ctx.state = RX_WAIT_MAGIC;
                break;
            }

            rx_ctx.state = RX_READ_PAYLOAD;
            break;

        /* ---------------- READ PAYLOAD ---------------- */
        case RX_READ_PAYLOAD: {
            /* Prevent overflow in case of corruption */
            if (rx_ctx.offset > rx_ctx.cur_msg.hdr.len) {
                LOG_ERR("RX offset overflow → resync");
                rx_ctx.state = RX_WAIT_MAGIC;
                rx_ctx.msg_active = 0;
                break;
            }
            uint16_t remaining_msg = rx_ctx.cur_msg.hdr.len - rx_ctx.offset;
            uint16_t remaining_pkt = len - i;

            uint16_t chunk = (remaining_msg < remaining_pkt)
                                ? remaining_msg
                                : remaining_pkt;

            memcpy(&rx_ctx.cur_msg.payload[rx_ctx.offset], &pkt[i], chunk);

            rx_ctx.offset += chunk;
            i += chunk;

            /* Message complete */
            if (rx_ctx.offset >= rx_ctx.cur_msg.hdr.len) {

                add_ble_msg_to_queue(&rx_ctx.cur_msg, BLE_RX);

                rx_ctx.msg_active = 0;
                rx_ctx.state = RX_WAIT_MAGIC;
            }

            break;
        }

        default:
            rx_ctx.state = RX_WAIT_MAGIC;
            break;
        }
    }
}

static void dump_ble_packet(const uint8_t *pkt, uint16_t len)
{
    if (!pkt || len == 0) {
        LOG_INF("EMPTY PACKET");
        return;
    }

    uint16_t i = 0;
    uint8_t msg_cont = pkt[i++];

    LOG_INF("BLE PACKET DUMP");
    LOG_INF("MSG_CONT: %u", msg_cont);

    /* =========================================================
     * STREAM MODE (BOTH CONTINUATION AND START BEHAVES SAME)
     * ========================================================= */
    while (i < len) {

        /* Wait for message boundary */
        if (pkt[i] != BLE_MAGIC) {

            LOG_INF("RAW [%u]: 0x%02X", i, pkt[i]);
            i++;
            continue;
        }

        /* ---- MESSAGE START ---- */
        i++; // skip MAGIC

        if (i + 3 > len) {
            LOG_INF("TRUNCATED HEADER");
            return;
        }

        uint8_t opcode  = pkt[i++];
        uint8_t req_app = pkt[i++];
        uint8_t msg_len = pkt[i++];

        LOG_INF("---- MESSAGE ----");
        LOG_INF("OPCODE  : 0x%02X", opcode);
        LOG_INF("REQ_APP : %u", req_app);
        LOG_INF("LEN     : %u", msg_len);

        uint16_t remaining = len - i;
        uint16_t available = (msg_len < remaining) ? msg_len : remaining;

        LOG_INF("PAYLOAD:");

        for (uint16_t k = 0; k < available; k++) {
            LOG_INF("  [%u] 0x%02X", k, pkt[i + k]);
        }

        if (available < msg_len) {
            LOG_INF("  ... TRUNCATED (%u/%u)", available, msg_len);
        }

        i += available;
    }

    LOG_INF("END PACKET");
}

/**
 * @brief This is where the phone TX writes packets <br/>
 *
 * @param conn  BLE handle
 * @param attr  GATT attribute
 * @param buf   Buffer to read from
 * @param len   Length to read upto
 * @param offset    Start of read 
 * @param flags Indicates if its a part of a message 
 * or the full message, BT_GATT_WRITE_FLAG_PREPARE means there is
 * more data to follow, while BT_GATT_WRITE_FLAG_EXECUTE means finished
 *
 * @return 
 */
static ssize_t mw_rx_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{

    LOG_INF("Write RX packet len=%u", len);

    if (!buf || len == 0) {
        return len;
    }
    
    // dump_ble_packet((const uint8_t *)buf, len);
    ble_retrieve_packet((const uint8_t *)buf, len);
    return len;
}

static void mw_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	notifications_enabled = (value == BT_GATT_CCC_NOTIFY);
	LOG_INF("Notifications %s", notifications_enabled ? "enabled" : "disabled");
}

static void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
    LOG_INF("Capabale MTU: Central %u, Peripheral %u bytes", tx, rx);
    effective_mtu = bt_gatt_get_mtu(conn);
    effective_mtu = effective_mtu >=3 ? effective_mtu -3 : 0;
    LOG_INF("Effective updated MTU : %u", effective_mtu);
}

BT_GATT_SERVICE_DEFINE(mw_svc,
    BT_GATT_PRIMARY_SERVICE(&mw_service_uuid),

    /* TX Char: Peripheral -> Central (Notifications) */
    BT_GATT_CHARACTERISTIC(&tx_chrc_uuid.uuid,
        BT_GATT_CHRC_NOTIFY,
        BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(mw_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    /* RX Char: Central -> Peripheral (Write Without Response) */
    BT_GATT_CHARACTERISTIC(&rx_chrc_uuid.uuid,
        BT_GATT_CHRC_WRITE_WITHOUT_RESP,
        BT_GATT_PERM_WRITE, NULL, mw_rx_write_cb, NULL),
);

void populate_tx_buffers() {
    if(!(ble_conn && ble_connected && notifications_enabled)) {
        return;
    }

    uint16_t len;
    while (ble_build_next_packet(&len)) {
        if (ble_conn && ble_connected && notifications_enabled) {
            int err = bt_gatt_notify(ble_conn, &mw_svc.attrs[2], tx_buffer, len);
            if (err) {
                LOG_ERR("Notify failed: %d", err);
            } else {
                LOG_INF("Notify success, len=%d", len);
            }
        }
    }  

}

void process_rx_buffers() {
    ble_msg_t msg;
    while(get_next_ble_msg(&msg, BLE_RX)){
        handle_ble_response(&msg);
    }
}

static void rx_reset_context(void)
{
    rx_ctx.state = RX_WAIT_MAGIC;
    rx_ctx.offset = 0;
    rx_ctx.msg_active = 0;
}

static void connected(struct bt_conn *conn, uint8_t err) {
    if(err) {
        LOG_ERR("BLE conn failed : %s", bt_hci_err_to_str(err));
        return;
    }
    if(ble_conn) {
        LOG_ERR("Multiple connections detected. Rejecting latest");
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        return;
    }

    LOG_INF("Connected");
    ble_connected = true;
    
    ble_conn = bt_conn_ref(conn);
    rx_reset_context();

    int param_err = bt_conn_le_param_update(conn, &conn_params);
    if(param_err) 
        LOG_ERR("Error updating params : %d", param_err);

    /*int sec_err = bt_conn_set_security(conn, BT_SECURITY_L2);
    if(sec_err) {
        LOG_ERR("Failed to initate auth : %d", sec_err);
    }*/

}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    LOG_INF("Disconnected %d %s", reason, bt_hci_err_to_str(reason));
    ble_connected = false;
    notifications_enabled = false;
    if(ble_conn) {
        bt_conn_unref(ble_conn);
        ble_conn = NULL;
    }
    tx_msg_active = 0;
    tx_offset = 0;
    rx_reset_context();
}

static void recycle(void) {
    int err = bt_le_adv_start(&adv_param_factory, ad, ARRAY_SIZE(ad), NULL, 0);
    if(err) {
        LOG_ERR("Advertising failed : %d", err);
    }     
    else {
        LOG_INF("Advertising started");
    } 
}

static void params_updated(struct bt_conn* conn, uint16_t interval,
                            uint16_t latency, uint16_t timeout) {
    LOG_INF("Updated params to : %u %u %u", interval, latency, timeout);
    // Update new starting anchor point here  
}

/*
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey) {
    LOG_INF("Generated passkey is %06u " , passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
    bt_conn_auth_passkey_confirm(conn);
    LOG_INF("Auto-confirmed passkey: %06u", passkey);
}

static void auth_cancelled(struct bt_conn *conn) {
    LOG_ERR("Cancelled auth during pairing");
} 

static void auth_pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    LOG_ERR("Pairing failed (reason %d)", reason);
}

void paired(struct bt_conn *conn, bool bonded) {
    LOG_INF("Paired");
}

static struct bt_conn_auth_info_cb bt_conn_auth_info = {
	.pairing_complete = paired,
    .pairing_failed = auth_pairing_failed 
};

static struct bt_conn_auth_cb auth_cb_display = {
    //.passkey_display = auth_passkey_display,
    .passkey_display = NULL,
    .passkey_entry = NULL,
    .passkey_confirm = NULL,
    .cancel = auth_cancelled 
};
*/

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .recycled = recycle,
    .le_param_updated = params_updated 
};

static struct bt_gatt_cb gatt_callbacks = {
    .att_mtu_updated = mtu_updated,
};



void init_ble() {
    int err; 
    err = bt_enable(NULL);
    if(err) {
        LOG_ERR("Failed enabling BT : %d", err);
    }
    /*
    err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
    if(err) LOG_ERR("Failed unpairing %d", err);

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

    bt_conn_auth_cb_register(&auth_cb_display);
    */
    
    bt_gatt_cb_register(&gatt_callbacks);

    err = bt_le_adv_start(&adv_param_factory, ad, ARRAY_SIZE(ad), NULL, 0);
    if(err) {
        LOG_ERR("Advertising failed : %d", err);
    }     
    else {
        LOG_INF("Advertising started");
    } 
	//bt_conn_auth_info_cb_register(&bt_conn_auth_info);


}
