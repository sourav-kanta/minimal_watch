#ifndef UI_MANAGER_H
#define UI_MANAGER_H


#include <lvgl.h>
#include "ui_types.h"


void init_ui();
void forward_wf_event(event_t*);
ui_state_t get_current_ui_state();
display_state_t get_current_display_state();
void deinit_ui();
void remove_shadow_and_outline();
void make_obj_navigable();
watchface_t* get_selected_wf();
void update_notification_ui();

#endif
