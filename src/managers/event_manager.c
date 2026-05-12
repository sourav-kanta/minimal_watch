/**
 * @file event_manager.c
 * @brief Handle registration and drive notification for watch events
 * @author Sourav Kanta
 * @version v1
 * @date 2026-04-08
 */
#include "managers/event_manager.h"
#include "managers/ble_manager.h"
#include "managers/ui_manager.h"
#include "event_types.h"
#include "common_types.h"
#include "ble_types.h"
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include "managers/app_manager.h"
#include <time.h>

LOG_MODULE_REGISTER(EVENT_MANAGER, LOG_LEVEL_INF);

void handle_ble_response(ble_msg_t* msg) {
    switch(msg->hdr.opcode) {
        case BLE_OP_TIME_UPDATE :
            if(msg->hdr.len >= 4) {
                uint32_t epoch = ((uint32_t)msg->payload[0] << 24) |
                     ((uint32_t)msg->payload[1] << 16) |
                     ((uint32_t)msg->payload[2] << 8)  |
                     ((uint32_t)msg->payload[3] << 0);
                struct tm curr_time;
                time_t time_val = (time_t) epoch;
                if (gmtime_r(&time_val, &curr_time) != NULL) {
                    LOG_INF("Updating time to : %d-%02d-%02d : %02d:%02d:%02d",
                                                curr_time.tm_year + 1900,
                                                curr_time.tm_mon + 1,
                                                curr_time.tm_mday, curr_time.tm_hour,
                                                curr_time.tm_min, curr_time.tm_sec);
                    date_time_t date_now = {
                        .day = curr_time.tm_mday,
                        .month = curr_time.tm_mon + 1,
                        .year = curr_time.tm_year + 1900,
                        .hr = curr_time.tm_hour,
                        .min = curr_time.tm_min,
                        .sec = curr_time.tm_sec 
                    };

                }
            }
            else {
                LOG_ERR("Invalid time update message");
            }
            break;
    }
}

void handle_event(event_t* event) {
    switch(event->ev) {
        case BLE : 
            break;
        case TICK_UPDATE :
            
            // Testing for ble 
            populate_tx_buffers();
            process_rx_buffers();
            // End Test
            
            forward_wf_event(event); 
            break;
        case SENSOR_DATA :
            break;
        default :
            LOG_ERR("Unknown event received");
            break;
    }

}

static uint8_t generate_app_id() {
    ui_state_t ui_state = get_current_ui_state();
    switch(ui_state) {
        case WATCHFACE :
            // TODO replace with watchface-id + 1
            if(get_selected_wf() == NULL) 
                return 0; 
            return get_selected_wf()->wf_id + 1;
        case APP : 
            return MAX_WATCHFACES + 1 + get_curr_app()->app_id;
        default :
            return 0;
    }
}



bool request_ble_action(ble_req_t req) {
    bool success = true;
    switch(req.req_code) {
        case UPDATE_SYSTEM_TIME : {
            uint8_t app_id = generate_app_id();
            if(app_id == 0) {
                success = false;
                break;
            }
            ble_msg_t new_msg = {
                .hdr = { 
                    .opcode = BLE_OP_TIME_UPDATE,
                    .len = 0,
                    .req_app = generate_app_id()
                } 
            };
            success = add_ble_msg_to_queue(&new_msg, BLE_TX);
            break;     
        }
        default :
            success = false;       
    }
    return success;
}

void init_events() {
    ble_req_t time_sync = { .req_code = UPDATE_SYSTEM_TIME }; 
    request_ble_action(time_sync);
}

//int request_sensor_read(sensor_req_t) {
//
//}
