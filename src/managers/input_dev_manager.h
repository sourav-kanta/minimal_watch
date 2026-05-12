/**
 * @file input_dev_manager.h
 * @brief Expose input device apis
 * @author Sourav Kanta
 * @version v1
 * @date 2026-04-10
 */


#ifndef INPUT_DEV_MANAGER_H
#define INPUT_DEV_MANAGER_H

#include "lvgl.h"

void setup_keyboard();
void global_input_filter_cb(lv_event_t*);

#endif 
