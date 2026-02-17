#include <zephyr/logging/log.h>
#include "ui/appmgr_ui.h"
#include <managers/app_manager.h>

LOG_MODULE_REGISTER(appmgr_ui, LOG_LEVEL_INF);

static lv_coord_t col_dsc[] = {LV_GRID_FR(1),
                               LV_GRID_FR(1),
                               LV_GRID_FR(1),
                               LV_GRID_TEMPLATE_LAST};
lv_coord_t *row_dsc;

static lv_obj_t* app_root_cont = NULL;
static lv_obj_t* app_scr = NULL;

/**
 * @brief Handle gestures on app screen. We only want to 
 * handle the left swipe gesture in here, every thing else 
 * should be discarded as app is currently active
 *
 * @param ev Event we receive on the wrapper container
 */
void handle_app_scr_gestures(lv_event_t* ev) {
    lv_event_code_t code = lv_event_get_code(ev);
    lv_indev_t *inp_dev = lv_indev_active();
    if(code == LV_EVENT_GESTURE) {
        lv_dir_t ges_dir = lv_indev_get_gesture_dir(
                                inp_dev);
        switch(ges_dir) {
            case LV_DIR_LEFT :
                LOG_INF("Left gesture detected in app");
                close_curr_app();
                lv_obj_delete(app_scr);
                app_scr = NULL;
                break;
            default:
                LOG_INF("Discarding event as user in app");
                break;
        }
        lv_indev_wait_release(inp_dev);
    }
}

/**
 * @brief Removes border, padding and outline
 *
 * @param obj
 */
void remove_shadow_and_outline(lv_obj_t* obj) {
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, 0,LV_PART_MAIN);
    lv_obj_set_style_pad_row(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(obj, 0, LV_PART_MAIN);

}

/**
 * @brief Create a new object over the wrapper container
 * to display the app. Registers a event handler for back
 * gestures in app and prevents the event from bubbling to 
 * parent (root screen handler)
 *
 * @return Newly created app screen
 */
lv_obj_t* create_app_screen() {
    if(app_root_cont == NULL) {
        LOG_ERR("App screen is null");
        return NULL;
    }
    else {
        app_scr = lv_obj_create(app_root_cont);
        remove_shadow_and_outline(app_scr);
        lv_obj_set_size(app_scr, lv_pct(100), lv_pct(100));
        //lv_obj_add_flag(app_scr, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(app_scr, handle_app_scr_gestures,
                            LV_EVENT_GESTURE, NULL);
        lv_obj_move_foreground(app_scr);
        lv_obj_clear_flag(app_scr,
                LV_OBJ_FLAG_GESTURE_BUBBLE);
        return app_scr;
    }
}

/**
 * @brief Draws the app selection UI
 *
 * @param root Root screen
 * @param apps List of all applications
 * @param num_apps Total number of applications
 */
void draw_app_manager_ui(lv_obj_t* root,
                        application* *apps,
                        uint8_t num_apps) {
    // Set number of rows in grid
    int total_rows = num_apps % 3 == 0 ? 
                     num_apps / 3 :
                     (num_apps / 3) + 1;
    row_dsc = lv_malloc(sizeof(lv_coord_t) *
                        (total_rows + 1));
    for(int i=0; i<total_rows; i++) {
        row_dsc[i] = 80;
    }
    row_dsc[total_rows] = LV_GRID_TEMPLATE_LAST;

    // Wrapper container 
    app_root_cont = lv_obj_create(root);
    lv_obj_set_size(app_root_cont, lv_pct(100), lv_pct(100));
    remove_shadow_and_outline(app_root_cont);

    // Main container
    lv_obj_t* parent_cont = lv_obj_create(app_root_cont);
    lv_obj_set_flex_flow(parent_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_size(parent_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_scrollbar_mode(parent_cont,
                              LV_SCROLLBAR_MODE_OFF);
    remove_shadow_and_outline(parent_cont);

    // App bar containing label
    lv_obj_t *app_bar = lv_obj_create(parent_cont);
    lv_obj_set_style_bg_color(app_bar,
            lv_palette_main(LV_PALETTE_GREY),
            LV_PART_MAIN);
    lv_obj_set_style_pad_all(app_bar, 5, LV_PART_MAIN);
    lv_obj_set_style_border_width(app_bar, 0, 
            LV_PART_MAIN);
    lv_obj_set_style_shadow_width(app_bar, 0, 
            LV_PART_MAIN);
    lv_obj_set_size(app_bar, lv_pct(100), 25);
    
    lv_obj_t *bar_label = lv_label_create(app_bar);
    lv_obj_set_size(bar_label, lv_pct(100), lv_pct(100));
    lv_label_set_text(bar_label, "Applications");
    lv_obj_center(bar_label);
    lv_obj_set_style_text_align(bar_label, 
            LV_TEXT_ALIGN_CENTER , LV_PART_MAIN);
    lv_obj_set_style_text_color(bar_label, 
            lv_color_hex(0xFFFFFF),
            LV_PART_MAIN);

    lv_obj_t *pad = lv_obj_create(parent_cont);
    lv_obj_set_size(pad, lv_pct(100), 5);
    lv_obj_set_style_outline_width(pad, 0 , LV_PART_MAIN);

    // Grid container
    lv_obj_t *cont = lv_obj_create(parent_cont);
    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);
    lv_obj_set_flex_grow(cont, 1);
    lv_obj_set_width(cont, LV_PCT(100));
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_border_width(cont, 0, 
            LV_PART_MAIN);
    lv_obj_set_style_shadow_width(cont, 0, 
            LV_PART_MAIN);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    lv_obj_set_style_pad_column(cont, 0, 0);

    for(int i = 0; i<num_apps; i++) {
        uint8_t col = i%3;
        uint8_t row = i/3;
        lv_obj_t* btn = lv_btn_create(cont);
        lv_obj_add_event_cb(btn, 
                            open_app, 
                            LV_EVENT_CLICKED,
                            apps[i]);
        lv_obj_set_layout(btn, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_bg_color(btn,
                lv_color_hex(0xFFFFFF),
                LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(btn, 0, 
                LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn, 0, 
                LV_PART_MAIN);
        lv_obj_set_grid_cell(btn, 
                             LV_GRID_ALIGN_STRETCH, col, 1,
                             LV_GRID_ALIGN_STRETCH, row, 1);
    
        lv_obj_t *icon = lv_image_create(btn);
        lv_image_set_src(icon, apps[i]->ico);
        lv_obj_set_size(icon, 50, 50);
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 0);
        
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, apps[i]->name);
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 5);
        lv_obj_set_style_text_color(label,
                    lv_color_hex(0x0),
                    LV_PART_MAIN);
    }
    lv_obj_update_layout(cont);
}

/**
 * @brief Delete the app manager UI. Need to delete the 
 * allocated memory for gridview structures
 *
 * @param root
 */
void del_app_manager_ui(lv_obj_t* root) {
    LOG_INF("Freeing the row_dsc object");
    lv_free(row_dsc);
    app_root_cont = NULL;
}

