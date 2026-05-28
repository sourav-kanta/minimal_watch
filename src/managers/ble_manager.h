#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <stdbool.h>
#include "ble_types.h"

void init_ble();
bool submit_ble_request(const ble_req_t*, const uint8_t);
void populate_tx_buffers();
void process_rx_buffers();

#endif
