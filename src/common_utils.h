#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include "common_types.h"
#include <lvgl.h>

void get_date_time(date_time_t*);
void get_curr_weather(hourly_weather_t*);
uint8_t generate_curr_app_id();
void get_weather_day(hourly_weather_t*);
void make_obj_navigable(lv_obj_t*);
void remove_shadow_and_outline(lv_obj_t*);
void request_ble_resource(app_ble_req_t, void *);

#endif
