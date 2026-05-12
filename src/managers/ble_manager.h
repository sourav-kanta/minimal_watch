#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <stdbool.h>
#include "ble_types.h"

void init_ble();
bool add_ble_msg_to_queue(const ble_msg_t*, const ble_comm_type_t);
void populate_tx_buffers();
void process_rx_buffers();

#endif
