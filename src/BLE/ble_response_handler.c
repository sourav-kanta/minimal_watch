
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

/**
 * @brief Safely extracts a variable length string field from a payload buffer.
 *
 * @param[in]     payload      Pointer to the raw BLE payload stream.
 * @param[in]     payload_len  The total size of the message payload.
 * @param[in,out] current_idx  Pointer to the current extraction index tracking.
 * @param[out]    dest_buf     Pointer to the destination buffer array inside the struct.
 * @param[in]     dest_max     The absolute maximum size allocated for dest_buf (including \0).
 *
 * @return int 0 on success, negative error code on bounds violations.
 */
static int extract_lean_string(const uint8_t *payload, 
                               uint16_t payload_len, uint16_t *current_idx,
                               char *dest_buf, uint8_t dest_max) {
    uint16_t idx = *current_idx;

    if (idx >= payload_len) {
        return -EBADMSG;
    }

    uint8_t str_len = payload[idx++];

    if (idx + str_len > payload_len) {
        return -EMSGSIZE;
    }

    uint8_t copy_bytes = (str_len < (dest_max - 1)) ? str_len : (dest_max - 1);

    memcpy(dest_buf, &payload[idx], copy_bytes);
    dest_buf[copy_bytes] = '\0'; // Force absolute safe string termination

    *current_idx = idx + str_len;
    return 0;
}

/**
 * @brief This function extracts dynamic variable-length notification metrics transmitted over BLE.
 *
 * @param[in]  payload          Pointer to the source buffer containing raw incoming bytes.
 * Must be verified to be at least 5 bytes long before calling.
 * @param[in]  len              Total length of the payload parsed from the BLE packet header.
 * @param[out] dest             Pointer to the destination notification structure where the
 * unpacked fields will be saved.
 *
 * @verbatim
 * ====================================================================================
 * DYNAMIC PAYLOAD LENGTH-VALUE BYTE LAYOUT MAP
 * ====================================================================================
 * Total Stream Size: Variable (Leaned dynamic string pack)
 * Layout uses an explicit length byte preceding every string parameter.
 * No structural alignment holes or padding bytes are present.
 *
 * 1. ENUM IDENTIFIER (1 Byte)
 * [0]         : app           (uint8_t  - Maps directly to phone_app_t enum)
 *
 * 2. VARIABLE TEXT STRING STREAM
 * Because strings are dynamic, elements are accessed sequentially via tracking heads.
 * Each text section follows a [1-byte length field] + [N-bytes data payload] pattern.
 * * Layout sequence per string block:
 * [+0]        : string_len    (uint8_t  - Characters count from Flutter environment)
 * [+1 - +N]   : string_bytes  (char[]   - Raw UTF-8 bytes, no trailing '\0' over air)
 *
 * Stream Processing Sequence Order:
 * - Extract App ID (Byte 0) -> Advanced index head to 1
 * - Read App Name len       -> Extract App Name string  -> Move index head
 * - Read Msg Body len       -> Extract Msg Body string  -> Move index head
 * - Read Action Text len    -> Extract Action Text string -> Move index head
 * - Read Dismiss Text len   -> Extract Dismiss Text string -> Match final len
 * ====================================================================================
 * @endverbatim
 */

/**
 * @brief Parses a lean dynamic Length-Value packet into a notification_t structure.
 */
static int parse_notification_payload(const uint8_t *payload, uint16_t len, notification_t *dest) {
    uint16_t idx = 0;
    int err;

    // Check baseline minimum: 1-byte app ID + 4 tracking lengths
    if (len < 5) {
        return -EINVAL;
    }

    dest->app = (phone_app_t)payload[idx++];

    err = extract_lean_string(payload, len, &idx, dest->app_name, 14);
    if (err < 0) return err;

    err = extract_lean_string(payload, len, &idx, dest->body, 100);
    if (err < 0) return err;

    err = extract_lean_string(payload, len, &idx, dest->action_name, 15);
    if (err < 0) return err;

    err = extract_lean_string(payload, len, &idx, dest->dismiss_text, 15);
    if (err < 0) return err;

    if (idx != len) {
        LOG_WRN("Payload tail checking warning: parsed %u bytes, expected %u", idx, len);
    }

    dest->action_handler = NULL;
    dest->dismiss_handler = NULL;

    return 0;
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
            break;
        case BLE_OP_NOTIFICATION_SEND:
            if (msg->hdr.len >= 5) { 
                notification_t notification;
                int err = parse_notification_payload(msg->payload, msg->hdr.len, &notification);

                if (err == 0) {
                    LOG_INF("NOTIFICATION RECEIVED: [%s] Type: %d",
                              notification.app_name, notification.app);
                    LOG_INF("Body: %s", notification.body);
                    event_t event = {
                        .payload_len = sizeof(notification_t),
                        .data = &notification,
                        .ev = EVENT_NOTIFICATION_RECEIVED
                    };
                    handle_event(&event);
                } else {
                    LOG_ERR("Failed to decode variable notification payload: %d", err);
                }
            }
            else {
                LOG_ERR("Invalid notification message length");
            }
            break;
    }
}


