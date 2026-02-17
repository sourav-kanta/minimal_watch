#include <lvgl.h>
#include <zephyr/init.h>
#include "ui/watchface/wf_picker.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wf2, LOG_LEVEL_INF);

static watchface wf2;

void wf2_draw(lv_obj_t* root) {

    LOG_INF("Drawing wf2");

}

void wf2_update(lv_obj_t* root,
        wf_event event) {
}

void wf2_del_wf(lv_obj_t* root) {

}

static watchface wf2 = {
    .name = "LIGHT WF",
    .draw_watchface = &wf2_draw,
    .update_watchface = &wf2_update,
    .del_watchface = &wf2_del_wf
};

static int register_wf(void) {
    add_wf(&wf2);
    return 0;
}

SYS_INIT(register_wf, APPLICATION, WATCHFACE_PRIORITY); 
