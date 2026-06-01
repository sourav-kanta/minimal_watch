
#include <zephyr/logging/log.h>
#include "ble_types.h"
#include "managers/persistance_manager.h"
#include "managers/event_manager.h"

LOG_MODULE_REGISTER(BLE_RESPONSE_HANDLER, LOG_LEVEL_INF);

/**
 * @brief This function extracts bundled hourly_today weather metrics transmitted over BLE 
 *
 * @param[in]  payload        Pointer to the source buffer containing raw incoming bytes.
 *                            Must be verified to be at least 197 bytes long before calling.
 * @param[out] weather_state  Pointer to the destination destination structure where the 
 *                            unpacked fields will be saved.
 *
 * @verbatim
 * ====================================================================================
 *                         GLOBAL PAYLOAD LINEAR BYTE LAYOUT Map
 * ====================================================================================
 * Total Stream Size: 148 Bytes
 * No structural alignment holes or padding bytes are present.
 *
 * 1. HEADER METADATA (4 Bytes)
 *    [0 - 3]   : expires_at       (uint32_t - Big Endian Epoch seconds for 23:59:59)
 *
 * 2. HOURLY MATRIX: hourly_today[24] (144 Bytes Total)
 *    Each hourly element occupies exactly 6 bytes. 
 *    To access Hour H (0-23): Index Offset = 4 + (H * 6)
 *    
 *    Layout per hour:
 *    [+0 - +1] : temperature      (int16_t  - Scaled by 10, Big Endian)
 *    [+2]      : humidity         (uint8_t  - Percentage 0 to 100)
 *    [+3]      : precip_prob      (uint8_t  - Percentage 0 to 100)
 *    [+4]      : weather_code     (uint8_t  - WMO Weather Code 0 to 99)
 *    [+5]      : wind_speed       (uint8_t  - Rounded integer value)
 *
 * ====================================================================================
 * @endverbatim
 */
static void parse_weather_payload(const uint8_t *payload, weather_sync_t* weather_state) {
    uint16_t idx = 0;

    weather_state->expires_at = ((uint32_t)payload[idx]     << 24) |
                                ((uint32_t)payload[idx + 1] << 16) |
                                ((uint32_t)payload[idx + 2] << 8)  |
                                ((uint32_t)payload[idx + 3] << 0);
    idx += 4;
    
    for (int i = 0; i < 24; i++) {
        int16_t temp = (int16_t)((payload[idx] << 8) | payload[idx + 1]);
        weather_state->hourly_today[i].temperature = temp;
        idx += 2;

        weather_state->hourly_today[i].humidity     = payload[idx++];
        weather_state->hourly_today[i].precip_prob  = payload[idx++];
        weather_state->hourly_today[i].weather_code = payload[idx++];
        weather_state->hourly_today[i].wind_speed   = payload[idx++];
        LOG_DBG("Hour : %02d Humidity : %02u Precipitation : %02u \
                Weather code : %02u Wind speed %02u Temp %02d", i ,
                weather_state->hourly_today[i].humidity,     
                weather_state->hourly_today[i].precip_prob,  
                weather_state->hourly_today[i].weather_code, 
                weather_state->hourly_today[i].wind_speed,
                weather_state->hourly_today[i].temperature);   
    }
}

static void parse_date_time_payload(const uint8_t* payload) {
    uint32_t epoch = ((uint32_t)payload[0] << 24) |
         ((uint32_t)payload[1] << 16) |
         ((uint32_t)payload[2] << 8)  |
         ((uint32_t)payload[3] << 0);
    update_persistant_time_state(epoch);
}

void handle_ble_response(const ble_msg_t* msg) {
    switch(msg->hdr.opcode) {
        case BLE_OP_TIME_UPDATE :
            if(msg->hdr.len == 4) {
                parse_date_time_payload(msg->payload);
            }
            else {
                LOG_ERR("Invalid time update message");
            }
            break;
        case BLE_OP_WEATHER_UPDATE :
            if(msg->hdr.len ==148) {
                weather_sync_t weather_state;
                parse_weather_payload(msg->payload, &weather_state);
                update_persistant_weather_state(&weather_state);
            }
            else {
                LOG_ERR("Invalid weather message");
            }
            break;
        case BLE_OP_DATED_WEATHER_QUERY :
            if(msg->hdr.len ==148) {
                weather_sync_t weather_state;
                parse_weather_payload(msg->payload, &weather_state);
                app_update_t update = {
                    .req = DATED_WEATHER_REQUEST,
                    .req_app = msg->hdr.req_app,
                };
                memcpy(update.data, weather_state.hourly_today, sizeof(hourly_weather_t)*24);
                event_t event = {
                    .payload_len = sizeof(update),
                    .data = &update,
                    .ev = EVENT_APP_WORK_SCHEDULE 
                };
                handle_event(&event);
            }
            else {
                LOG_ERR("Invalid weather message");
            }
    }
}


