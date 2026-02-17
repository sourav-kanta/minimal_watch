#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include <lvgl.h>
#include <common_types.h>
#include <zephyr/init.h>

void show_app_picker_ui(lv_obj_t*);
void add_app(application*);
void open_app(lv_event_t*);
void close_curr_app();

#endif
