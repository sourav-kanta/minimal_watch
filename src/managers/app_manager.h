/**
 * @file app_manager.h
 * @brief APIs for handling apps
 * @author Sourav Kanta
 * @version v1
 * @date 2026-04-12
 */
#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include <lvgl.h>
#include <common_types.h>
#include <zephyr/init.h>
#include <zephyr/sys/atomic.h>

void show_app_picker_ui(lv_obj_t*);
void add_app(application_t*);
void open_app(lv_event_t*);
void close_curr_app();
bool check_if_app_running();
void send_app_update(app_update_t*, atomic_t*);
application_t* get_curr_app();

#endif
