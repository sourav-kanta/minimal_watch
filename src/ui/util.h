#ifndef UI_UTILS_MANAGER_H
#define UI_UTILS_MANAGER_H

#include <lvgl.h>

void remove_shadow_and_outline();
void make_obj_navigable();
lv_obj_t* find_first_focusable_bfs(lv_obj_t*);

#endif
