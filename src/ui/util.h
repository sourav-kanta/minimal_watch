#ifndef UI_UTILS_MANAGER_H
#define UI_UTILS_MANAGER_H

#include <lvgl.h>

void remove_shadow_and_outline();
void make_obj_navigable(lv_obj_t*);
lv_obj_t* find_first_focusable_child_dfs(lv_obj_t*);
lv_obj_t* find_first_focusable_parent_dfs(lv_obj_t*);
lv_obj_t* find_next_focusable_sibling(lv_obj_t*);
lv_obj_t* find_prev_focusable_sibling(lv_obj_t*);

#endif
