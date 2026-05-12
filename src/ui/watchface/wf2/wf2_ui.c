#include <lvgl.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include "ui/watchface/wf_picker.h"
#include "common_types.h"

LOG_MODULE_REGISTER(wf2, LOG_LEVEL_INF);

static watchface_t wf2;

static lv_obj_t *lbl_batt, *lbl_day, *lbl_temp, *lbl_kcal, *lbl_steps,
         *lbl_date_num, *lbl_month, *lbl_time;

static bool odd_tick=true;

void wf2_draw(lv_obj_t* root) {
    // --- TACTICAL GREEN PALETTE ---
    lv_color_t c_grad_top = lv_color_hex(0x244230); // Medium Moss
    lv_color_t c_grad_bot = lv_color_hex(0x0F1B15); // Black-Green
    lv_color_t c_panel    = lv_color_hex(0x1B3125); // Dark Moss
    lv_color_t c_accent   = lv_color_hex(0x2ECC71); // Tactical Green
    lv_color_t c_white    = lv_color_hex(0xFFFFFF);

    // Lock screen to prevent scrolling
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(root, c_grad_top, 0);
    lv_obj_set_style_bg_grad_color(root, c_grad_bot, 0);
    lv_obj_set_style_bg_grad_dir(root, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);

    // --- TOP SECTION ---
    lbl_temp = lv_label_create(root);
    lv_label_set_text(lbl_temp, "24C");
    lv_obj_set_style_text_font(lbl_temp, &lv_font_montserrat_14, 0); 
    lv_obj_set_style_text_color(lbl_temp, c_white, 0);
    lv_obj_align(lbl_temp, LV_ALIGN_TOP_LEFT, 10, 10);

    lbl_time = lv_label_create(root);
    lv_label_set_text(lbl_time, "10:08");
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_24, 0); 
    lv_obj_set_style_text_color(lbl_time, c_white, 0);
    lv_obj_align(lbl_time, LV_ALIGN_TOP_RIGHT, -10, 8);

    // --- DATE BLOCK ---
    lbl_day = lv_label_create(root);
    lv_label_set_text(lbl_day, "MONDAY ");
    lv_obj_set_style_text_font(lbl_day, &lv_font_montserrat_10, 0); 
    lv_obj_set_style_text_color(lbl_day, c_white, 0); 
    lv_obj_align(lbl_day, LV_ALIGN_TOP_RIGHT, -30, 32);

    lbl_date_num = lv_label_create(root);
    lv_label_set_text(lbl_date_num, "26");
    lv_obj_set_style_text_font(lbl_date_num, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_date_num, c_accent, 0); 
    lv_obj_align(lbl_date_num, LV_ALIGN_TOP_RIGHT, -10, 32);

    lbl_month = lv_label_create(root);
    lv_label_set_text(lbl_month, "DECEMBER");
    lv_obj_set_style_text_font(lbl_month, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_month, c_white, 0);
    lv_obj_align(lbl_month, LV_ALIGN_TOP_RIGHT, -10, 44);

    // --- ACTIVITY PANEL (Reference for Battery) ---
    lv_obj_t * panel = lv_obj_create(root);
    lv_obj_set_size(panel, 118, 52); 
    lv_obj_align(panel, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_color(panel, c_panel, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_radius(panel, 8, 0);
    lv_obj_set_style_pad_all(panel, 6, 0);
    lv_obj_set_style_pad_row(panel, 3, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    // Steps Row
    lbl_steps = lv_label_create(panel);
    lv_label_set_text(lbl_steps, "STEPS 7645");
    lv_obj_set_style_text_font(lbl_steps, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_steps, c_white, 0);

    lv_obj_t * bar_steps = lv_bar_create(panel);
    lv_obj_set_size(bar_steps, 100, 4);
    lv_bar_set_value(bar_steps, 75, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_steps, c_accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar_steps, lv_color_hex(0x2A2E35), LV_PART_MAIN);

    // Kcal Row
    lbl_kcal = lv_label_create(panel);
    lv_label_set_text(lbl_kcal, "KCAL 328");
    lv_obj_set_style_text_font(lbl_kcal, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_kcal, c_white, 0);

    lv_obj_t * bar_kcal = lv_bar_create(panel);
    lv_obj_set_size(bar_kcal, 100, 4);
    lv_bar_set_value(bar_kcal, 40, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_kcal, c_accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar_kcal, lv_color_hex(0x2A2E35), LV_PART_MAIN);

    // --- BATTERY ARC (Aligned ABOVE Panel) ---
    lv_obj_t * arc = lv_arc_create(root);
    lv_obj_set_size(arc, 34, 34); 
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_value(arc, 75);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);   
    lv_obj_set_style_arc_width(arc, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x2A2E35), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, c_accent, LV_PART_INDICATOR);
    
    // Aligns the arc to the TOP-LEFT of the activity box with 4px vertical padding
    lv_obj_align_to(arc, panel, LV_ALIGN_OUT_TOP_LEFT, 5, -4);

    lbl_batt = lv_label_create(root); 
    lv_label_set_text(lbl_batt, "75");
    lv_obj_set_style_text_font(lbl_batt, &lv_font_montserrat_10, 0); // Using 10
    lv_obj_set_style_text_color(lbl_batt, c_white, 0);
    lv_obj_align_to(lbl_batt, arc, LV_ALIGN_CENTER, 0, 0);
}

void wf2_update(event_t *event) {
    LOG_INF("1s ticks received");
    if(event->ev == TICK_UPDATE) {
        odd_tick = odd_tick == true ? false : true;
        if(odd_tick)
           lv_label_set_text(lbl_time, "10 08");
        else
           lv_label_set_text(lbl_time, "10:08"); 
    }
}

void wf2_del_wf(lv_obj_t* root) {

}

static watchface_t wf2 = {
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
