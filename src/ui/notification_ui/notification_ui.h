#ifndef NOTIFICATION_UI_H
#define NOTIFICATION_UI_H

#include <lvgl.h>

void draw_notification_ui(lv_obj_t*);
void notification_ui_invalidate();
void del_notification_ui(lv_obj_t*);

#endif /* NOTIFICATION_UI_H */
