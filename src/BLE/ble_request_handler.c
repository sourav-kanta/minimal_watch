#include <zephyr/logging/log.h>
#include "managers/ble_manager.h"
#include "BLE/ble_fifo.h"
#include "ble_types.h"

LOG_MODULE_REGISTER(BLE_REQUEST_HANDLER, LOG_LEVEL_INF);

bool handle_ble_request(const ble_req_t *req, uint8_t app_id) {
    bool success = false;
    switch(req->req_code) {
        case UPDATE_SYSTEM_TIME : {
            ble_msg_t new_msg = {
                .hdr = { 
                    .opcode = BLE_OP_TIME_UPDATE,
                    .len = 0,
                    .req_app = app_id 
                } 
            };
            success = add_ble_msg_to_queue(&new_msg, BLE_TX);
            break;     
        }
        case UPDATE_SYSTEM_WEATHER : {
            ble_msg_t new_msg = {
                .hdr = { 
                    .opcode = BLE_OP_WEATHER_UPDATE,
                    .len = 0,
                    .req_app = app_id 
                } 
            };
            success = add_ble_msg_to_queue(&new_msg, BLE_TX);
            break;
        }
        case DATED_WEATHER_QUERY : {
            ble_msg_t new_msg = {
                .hdr = {
                    .opcode = BLE_OP_DATED_WEATHER_QUERY, 
                    .len = req->req_data_len,
                    .req_app = app_id 
                },
            };
            if(req->req_data_len <= MAX_BLE_PAYLOAD_SIZE)
                memcpy(new_msg.payload, req->req_data, req->req_data_len);    
            success = add_ble_msg_to_queue(&new_msg, BLE_TX);   
            break;
        }
        default :
            success = false;       
    }
    return success;
}
