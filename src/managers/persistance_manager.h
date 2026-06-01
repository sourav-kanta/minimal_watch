#ifndef PERSISTANCE_MANAGER_H
#define PERSISTANCE_MANAGER_H

#include "common_types.h"

void init_persistance();
time_sync_t get_curr_time_state();
const weather_sync_t* get_curr_weather_state();
void update_persistant_time_state(uint32_t);
void update_persistant_weather_state(weather_sync_t*);

#endif /* PERSISTANCE_MANAGER_H */
