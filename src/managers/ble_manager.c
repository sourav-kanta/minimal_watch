#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/addr.h>
#include "managers/ble_manager.h"
#include "ble_types.h"
#include "BLE/ble_request_handler.h"
#include "BLE/ble_response_handler.h"
#include "BLE/ble_fifo.h"
#include "BLE/ble_packet_handler.h"

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
    while (ble_build_next_packet(tx_buffer, effective_mtu, &len)) {
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

bool submit_ble_request(const ble_req_t* msg, const uint8_t app_id) {
    if(msg == NULL) {
        LOG_ERR("Invalid message, discarding!");
        return false;
    }
    else 
        return handle_ble_request(msg, app_id);
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
