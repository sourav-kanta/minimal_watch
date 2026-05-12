#ifndef BLE_TYPES_HEADER_H
#define BLE_TYPES_HEADER_H 

#include "common_types.h" 

#define MAX_BLE_PAYLOAD_SIZE 64
#define BLE_FIFO_SIZE 100

#define BLE_MAGIC 0xA5
#define BLE_HDR_SIZE 3

#define BT_UUID_SMARTWATCH_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

#define BT_UUID_RX_CHRC_VAL \
    BT_UUID_128_ENCODE(0x87654321, 0x4321, 0x8765, 0x4321, 0x876543210987)

#define BT_UUID_TX_CHRC_VAL \
    BT_UUID_128_ENCODE(0x87654321, 0x4321, 0x8765, 0x4321, 0x876543210988)

typedef uint8_t ble_opcode_t;

#define BLE_OP_TIME_UPDATE ((ble_opcode_t)0x01)

typedef enum {
    BLE_TX,
    BLE_RX
} ble_comm_type_t;

typedef enum {
    RX_WAIT_MAGIC,
    RX_READ_HEADER,
    RX_READ_PAYLOAD
} rx_state_t;

typedef struct {
    ble_opcode_t opcode;
    uint8_t req_app;
    uint8_t len;
} ble_msg_hdr_t;

/**
 * @brief Struct for populating the command and resopnses 
 * for BLE transmission. This is used only to maintain
 * track of requests and responses not for actual transmission
 * The Characteristic read callback will generate the actual
 * lean packet for each connection window
 */
typedef struct {
    ble_msg_hdr_t hdr;
    uint8_t payload[MAX_BLE_PAYLOAD_SIZE];
} ble_msg_t;

typedef enum {
    UPDATE_SYSTEM_TIME
} ble_req_type_t;


typedef struct ble_req {
    ble_req_type_t req_code;
    uint16_t req_data_len;
    uint8_t *req_data;
} ble_req_t; 

#endif
