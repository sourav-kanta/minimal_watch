#include "common_types.h"
#include "zephyr/logging/log.h"
#include "HAL/watch_state_storage.h"

LOG_MODULE_REGISTER(PERSISTANCE_MANAGER, LOG_LEVEL_INF);

static watch_state_t* watch_state = NULL;

void init_persistance() {
    assign_watch_state_addresses(&watch_state);
} 

time_sync_t get_curr_time_state() {
    if(watch_state == NULL) 
        LOG_ERR("Timestate not initialized. Hardware probably unrecognized");
    return watch_state->time_state;
}

const weather_sync_t* get_curr_weather_state() {
    if(watch_state == NULL) 
        LOG_ERR("Timestate not initialized. Hardware probably unrecognized");
    return &watch_state->weather_state;
}

void update_persistant_time_state(uint32_t epoch) {
    LOG_INF("Updating time");
    watch_state->time_state.last_sync_time = epoch;
    watch_state->time_state.time_sync_uptime = k_uptime_get()/1000;
    watch_state->time_state.valid = 1;
}

void update_persistant_weather_state(const weather_sync_t* weather) {
    if (weather == NULL) {
        return;
    }
    memcpy(&watch_state->weather_state, weather, sizeof(weather_sync_t));
}

