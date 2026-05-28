#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include "common_types.h"

void get_date_time(date_time_t*);
void get_curr_weather(hourly_weather_t*);
uint8_t generate_curr_app_id();

#endif
