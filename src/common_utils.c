#include <time.h>
#include "common_utils.h"
#include "managers/persistance_manager.h"
#include "zephyr/logging/log.h"
#include "managers/ui_manager.h"
#include "managers/app_manager.h"

LOG_MODULE_REGISTER(COMMON_UTILS, LOG_LEVEL_INF);

uint8_t generate_curr_app_id() {
    ui_state_t ui_state = get_current_ui_state();
    switch(ui_state) {
        case WATCHFACE :
            if(get_selected_wf() == NULL) 
                return 0; 
            return get_selected_wf()->wf_id + 1;
        case APP : 
            return MAX_WATCHFACES + 1 + get_curr_app()->app_id;
        default :
            return 0;
    }
}

void get_date_time(date_time_t* time) {
    const time_sync_t time_state = get_curr_time_state();    
    if(time_state.valid) {
        struct tm curr_time;
        time_t time_val = (time_t) (time_state.last_sync_time + 
                                    (k_uptime_get()/1000) -  
                                    time_state.time_sync_uptime);
        if (gmtime_r(&time_val, &curr_time) != NULL) {
            LOG_DBG("Updating time to : %d-%02d-%02d : %02d:%02d:%02d",
                                        curr_time.tm_year + 1900,
                                        curr_time.tm_mon + 1,
                                        curr_time.tm_mday, curr_time.tm_hour,
                                        curr_time.tm_min, curr_time.tm_sec);
           
            time->day = curr_time.tm_mday;
            time->month = curr_time.tm_mon + 1;
            time->year = curr_time.tm_year + 1900;
            time->hr = curr_time.tm_hour;
            time->min = curr_time.tm_min;
            time->sec = curr_time.tm_sec; 
            time->d_week = curr_time.tm_wday;    
        }
    }
    else {
        time->day = 1;
        time->month = 1;
        time->year = 1900;
        time->hr = 0;
        time->min = 0;
        time->sec = 0;
        time->d_week = 0;    
    }
}

void get_curr_weather(hourly_weather_t* curr_weather) {
    weather_sync_t weather = get_curr_weather_state();
    date_time_t curr_time;
    get_date_time(&curr_time);
    if(curr_time.hr >= 24 || curr_time.hr < 0) {
        LOG_ERR("Invalid current time");
        return;
    }
    LOG_DBG("Current temp : %02d", weather.hourly_today[curr_time.hr].temperature/10);
    memcpy(curr_weather, &weather.hourly_today[curr_time.hr], sizeof(hourly_weather_t));
}
