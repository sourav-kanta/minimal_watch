#ifndef APPMGR_UI_H
#define APPMGR_UI_H

#include "common_types.h"
#include <lvgl.h>

void draw_app_manager_ui(lv_obj_t*, application_t**, uint8_t);
void del_app_manager_ui(lv_obj_t*);
lv_obj_t* create_app_screen();
void handle_app_scr_close();

#endif
