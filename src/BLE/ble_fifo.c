#include "ble_types.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(BLE_FIFO, LOG_LEVEL_INF);

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
bool get_next_ble_msg(ble_msg_t *out_msg, const ble_comm_type_t type)
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

