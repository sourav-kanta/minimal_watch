#include <lvgl.h>
#include <zephyr/logging/log.h>
#include "ui/watchface/wf_picker.h"
#include "ui/watchface/wf1/assets/bg_img.h"
#include "ui/watchface/wf1/assets/min_img.h"

LOG_MODULE_REGISTER(wf1, LOG_LEVEL_INF);

static watchface wf1;
lv_obj_t * container=NULL;
lv_obj_t * bg_img=NULL;
lv_obj_t * min_hand_img=NULL;

void wf1_draw(lv_obj_t* root) {

    LOG_INF("Drawing wf1");
  
    container = lv_obj_create(root);
    lv_obj_set_size(container, 240, 240);
    lv_obj_center(container);      
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_background(container);
    
    bg_img = lv_image_create(container);
    lv_image_set_src(bg_img, &back_img);
    lv_obj_set_style_bg_opa(bg_img, LV_OPA_0, 0);
    lv_obj_center(bg_img);
    lv_obj_move_foreground(bg_img);
    
    // Min hand
    min_hand_img = lv_image_create(container);
    lv_image_set_src(min_hand_img, &min_img);
    lv_obj_set_pos(min_hand_img,100,24);
    lv_image_set_pivot(min_hand_img,5,80);
    lv_obj_set_style_bg_opa(min_hand_img, LV_OPA_0, 0);
    lv_obj_move_foreground(min_hand_img);
    
    LOG_INF("Finished drawing watchface"); 
}

void wf1_update(lv_obj_t* root,
        wf_event event) {
    switch(event.event_type) {
        case UI_WF_TIMER_UPDATE :
            date_time *data = event.data;
            int ang = data->sec*3600/60;
            lv_image_set_rotation(min_hand_img,ang);
    }
}

void wf1_del_wf(lv_obj_t* root) {

}

static watchface wf1 = {
    .name = "Moon WF",
    .draw_watchface = &wf1_draw,
    .update_watchface = &wf1_update,
    .del_watchface = &wf1_del_wf
};

static int register_wf(void) {
    add_wf(&wf1);
    return 0;
}

SYS_INIT(register_wf, APPLICATION, WATCHFACE_PRIORITY); 
