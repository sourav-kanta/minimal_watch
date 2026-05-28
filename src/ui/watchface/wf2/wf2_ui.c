#include <lvgl.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include "ui/watchface/wf_picker.h"
#include "ui_types.h"
#include "assets/back_image.h"

LOG_MODULE_REGISTER(wf2, LOG_LEVEL_INF);

static watchface_t wf2;

static lv_obj_t *lbl_batt, *lbl_day, *lbl_temp, *lbl_kcal, *lbl_steps,
         *lbl_date_num, *lbl_month, *lbl_time;

static const char month[12][13] = {"JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY",
                                   "JUNE", "JULY", "SEPTEMBER", "OCTOBER", "NOVEMBER",
                                   "DECEMBER"}; 

static const char day[7][10] = { "SUNDAY", " MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY",
                                 "FRIDAY", "SATURDAY"};

void wf2_draw(lv_obj_t* root) {
    lv_color_t c_panel    = lv_color_hex(0x1B3125); // Dark Moss
    lv_color_t c_accent   = lv_color_hex(0x2ECC71); // Tactical Green
    lv_color_t c_white    = lv_color_hex(0xFFFFFF);

    // Prevent screen scrolling
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    // --- ULTRA-LIGHT BACKGROUND IMAGE ---
    // Instead of creating a heavy generic container, we create the image 
    // directly on root and strip its layout features to save critical heap.
    lv_obj_t *bg_img = lv_image_create(root);
    lv_image_set_src(bg_img, &back_image);
    lv_obj_center(bg_img);
    
    // Disable hits/clicks on the image so LVGL doesn't track it in memory searches
    lv_obj_add_flag(bg_img, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_remove_flag(bg_img, LV_OBJ_FLAG_CLICKABLE);

    // Push the image to the absolute back of the render stack immediately
    lv_obj_move_background(bg_img);

    // --- TOP SECTION ---
    lbl_temp = lv_label_create(root);
    lv_label_set_text(lbl_temp, "24C");
    lv_obj_set_style_text_font(lbl_temp, &lv_font_montserrat_14, 0); 
    lv_obj_set_style_text_color(lbl_temp, c_white, 0);
    lv_obj_align(lbl_temp, LV_ALIGN_TOP_LEFT, 10, 10);

    lbl_time = lv_label_create(root);
    lv_label_set_text(lbl_time, "10:08");
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_14, 0); 
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

    // --- ACTIVITY PANEL ---
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

    lbl_steps = lv_label_create(panel);
    lv_label_set_text(lbl_steps, "STEPS 7645");
    lv_obj_set_style_text_font(lbl_steps, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_steps, c_white, 0);

    lv_obj_t * bar_steps = lv_bar_create(panel);
    lv_obj_set_size(bar_steps, 100, 4);
    lv_bar_set_value(bar_steps, 75, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_steps, c_accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar_steps, lv_color_hex(0x2A2E35), LV_PART_MAIN);

    lbl_kcal = lv_label_create(panel);
    lv_label_set_text(lbl_kcal, "KCAL 328");
    lv_obj_set_style_text_font(lbl_kcal, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_kcal, c_white, 0);

    lv_obj_t * bar_kcal = lv_bar_create(panel);
    lv_obj_set_size(bar_kcal, 100, 4);
    lv_bar_set_value(bar_kcal, 40, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_kcal, c_accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar_kcal, lv_color_hex(0x2A2E35), LV_PART_MAIN);

    // --- BATTERY ARC ---
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
    
    lv_obj_align_to(arc, panel, LV_ALIGN_OUT_TOP_LEFT, 5, -4);

    lbl_batt = lv_label_create(root); 
    lv_label_set_text(lbl_batt, "75");
    lv_obj_set_style_text_font(lbl_batt, &lv_font_montserrat_10, 0); 
    lv_obj_set_style_text_color(lbl_batt, c_white, 0);
    lv_obj_align_to(lbl_batt, arc, LV_ALIGN_CENTER, 0, 0);
}

void wf2_update(event_t *event) {
    if (event == NULL || event->data == NULL) {
        return;
    }

    if(event->ev == EVENT_TICK_UPDATE) {
        char time[16];
        char date[4];
        char temp[8];
        
        wf_update_payload_t* update_data = event->data;
        date_time_t* date_time = &update_data->time;
        hourly_weather_t* weather = &update_data->weather;

        uint8_t m_idx = (date_time->month > 0) ? (date_time->month - 1) : 0;
        if (m_idx >= 12) m_idx = 0;

        uint8_t d_idx = date_time->d_week;
        if (d_idx >= 7) d_idx = 0;

        snprintf(time, sizeof(time), "%02u:%02u:%02u", date_time->hr, 
                  date_time->min, date_time->sec);
        snprintf(date, sizeof(date), "%02u", date_time->day);
        snprintf(temp, sizeof(temp), "%2dC",  weather->temperature/10);
        
        if (lbl_time) lv_label_set_text(lbl_time, time);
        if (lbl_date_num) lv_label_set_text(lbl_date_num, date);
        if (lbl_month) lv_label_set_text(lbl_month, month[m_idx]);
        if (lbl_day) lv_label_set_text(lbl_day, day[d_idx]);
        if (lbl_temp) lv_label_set_text(lbl_temp, temp);
    }
}

void wf2_del_wf(lv_obj_t* root) {
    lbl_batt = lbl_day = lbl_temp = lbl_kcal = lbl_steps = NULL;
    lbl_date_num = lbl_month = lbl_time = NULL;
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

// Changed initialization priority to APPLICATION to run *after* system core stacks complete
SYS_INIT(register_wf, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
