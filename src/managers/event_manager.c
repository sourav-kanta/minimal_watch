/**
 * @file event_manager.c
 * @brief Handle registration and drive notification for watch events
 * @author Sourav Kanta
 * @version v1
 * @date 2026-04-08
 */
#include <time.h>
#include <zephyr/logging/log.h>
#include "managers/event_manager.h"
#include "managers/ble_manager.h"
#include "managers/ui_manager.h"
#include "managers/runtime_manager.h"
#include "event_types.h"
#include "common_types.h"
#include "ble_types.h"
#include "common_utils.h"
#include "ui_types.h"

LOG_MODULE_REGISTER(EVENT_MANAGER, LOG_LEVEL_INF);



void handle_event(event_t* event) {
    switch(event->ev) {
        case EVENT_TICK_UPDATE : {
            wf_update_payload_t payload;
            get_date_time(&payload.time);
            get_curr_weather(&payload.weather);
            event->data = &payload;
            forward_wf_event(event); 
            break;
        }
        case EVENT_WORK_TICK :
            display_state_t ui_state = get_current_display_state();
            curfew_hook_t cycle_hooks[] = { populate_tx_buffers };
            uint8_t hook_count = sizeof(cycle_hooks) / sizeof(cycle_hooks[0]);

            if(ui_state == ON) {
                set_runtime_state_with_hooks(STATE_UI_ACTIVE, cycle_hooks, hook_count);
            } else {
                set_runtime_state_with_hooks(STATE_BACKGROUND_ACTIVE, 
                        cycle_hooks, hook_count);
            }
            
            struct runtime_work_item ble_work = {
                .priority = 6,
                .handler = process_rx_buffers,
                .arg1 = NULL 
            };
            
            if(add_system_work(&ble_work) != 0) {
                LOG_ERR("Unable to add system work");
            }
            break;
        default :
            LOG_ERR("Unknown event received");
            break;
    }

}

bool request_ble_action(ble_req_t req) {
    bool success = true;
    uint8_t app_id = generate_curr_app_id();
    if(app_id == 0) {
        LOG_ERR("Invalid requesting app");
        success = false;
        return success;
    }
    success = submit_ble_request(&req, app_id);
    return success;
}

void init_events() {
    ble_req_t time_sync = { .req_code = UPDATE_SYSTEM_TIME }; 
    request_ble_action(time_sync);
    ble_req_t weather_sync = { .req_code = UPDATE_SYSTEM_WEATHER }; 
    request_ble_action(weather_sync);
}

//int request_sensor_read(sensor_req_t) {
//
//}
