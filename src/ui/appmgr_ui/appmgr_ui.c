#include <zephyr/logging/log.h>
#include "managers/ui_manager.h"
#include "ui/appmgr_ui/appmgr_ui.h"
#include <managers/app_manager.h>
#include "ui/util.h"
#include "ui/ui_theme.h"
#include "managers/input_dev_manager.h"

LOG_MODULE_REGISTER(appmgr_ui, LOG_LEVEL_INF);

static const int GRID_COLUMNS_PER_ROW = 2;
static const int GRID_ROW_HEIGHT_PX   = 80;

static const int UI_CONTAINER_WIDTH_PX  = 128;
static const int UI_CONTAINER_HEIGHT_PX = 160;
static const int UI_ICON_DIMENSION_PX   = 32;
static const int UI_LABEL_WIDTH_PX      = 60;
static const int ANIMATION_SPEED_MS       = 250;

static const int STYLE_BORDER_WIDTH_DEFAULT = 0;
static const int STYLE_BORDER_WIDTH_CELL    = 1;
static const int STYLE_BORDER_WIDTH_FOCUSED = 2;
static const int STYLE_PADDING_NONE         = 0;
static const int STYLE_OUTLINE_WIDTH_NONE   = 0;

static lv_coord_t col_dsc[] = {LV_GRID_FR(1),
                               LV_GRID_FR(1),
                               LV_GRID_TEMPLATE_LAST};
lv_coord_t *row_dsc;

static lv_obj_t* app_root_cont = NULL;
static lv_obj_t* app_scr = NULL;

void handle_app_scr_close() {
    LOG_INF("Need to close current app");
    if(app_scr == NULL) return;
    close_curr_app();
    lv_obj_delete(app_scr);
    app_scr = NULL;
}

void handle_app_scr_gestures(lv_event_t* ev) {
    lv_event_code_t code = lv_event_get_code(ev);
    lv_indev_t *inp_dev = lv_indev_active();
    if(code == LV_EVENT_GESTURE) {
        lv_dir_t ges_dir = lv_indev_get_gesture_dir(
                                inp_dev);
        switch(ges_dir) {
            case LV_DIR_LEFT :
                LOG_INF("Left gesture detected in app");
                handle_app_scr_close();
                break;
            default:
                LOG_INF("Discarding event as user in app");
                break;
        }
        lv_indev_wait_release(inp_dev);
    }
}

static void handle_app_screen_keys_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e); 
        lv_obj_t* curr_focused = lv_group_get_focused(get_current_group());
        if(key == LV_KEY_ESC) {
            if(!curr_focused) return;
            if(curr_focused == app_scr) {
                handle_app_scr_close();
            }
            else {
                lv_obj_t* parent = find_first_focusable_parent_dfs(curr_focused);
                if(parent) 
                    lv_group_focus_obj(parent);
                else
                    LOG_ERR("Unknown parent to navigate to");
            }
        }
        if(key == LV_KEY_ENTER) {
            if(!curr_focused) return;
            if(curr_focused == app_scr) {
                lv_obj_t* child = find_first_focusable_child_dfs(app_scr);
                if(child)
                    lv_group_focus_obj(child);
                else 
                    LOG_ERR("Unknown child to navigate to");
            }
        }
        if(key == LV_KEY_RIGHT) {
            LOG_INF("Finding next sibling");
            if(!curr_focused) return;
            if(curr_focused == app_scr) {
                lv_event_stop_bubbling(e); 
                return;
            }
            lv_obj_t* next_sibling = find_next_focusable_sibling(curr_focused);
            if(next_sibling)
                lv_group_focus_obj(next_sibling);
            else 
                LOG_ERR("Cant find next sibling");
        }
        if(key == LV_KEY_LEFT) {
            LOG_INF("Finding prev sibling");
            if(!curr_focused) return;
            if(curr_focused == app_scr) {
                lv_event_stop_bubbling(e); 
                return;
            }
            lv_obj_t* prev_sibling = find_prev_focusable_sibling(curr_focused);
            if(prev_sibling)
                lv_group_focus_obj(prev_sibling);
            else 
                LOG_ERR("Cant find previous sibling");
        }
    }
    lv_event_stop_bubbling(e);
}

lv_obj_t* create_app_screen() {
    if(app_root_cont == NULL) {
        LOG_ERR("App screen is null");
        return NULL;
    }
    else {
        app_scr = lv_obj_create(app_root_cont);
        remove_shadow_and_outline(app_scr);
        make_obj_navigable(app_scr);
        lv_group_focus_obj(app_scr);
        lv_obj_add_flag(app_scr, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_size(app_scr, lv_pct(100), lv_pct(100));
        lv_obj_add_event_cb(app_scr, handle_app_scr_gestures,
                            LV_EVENT_GESTURE , NULL);
        lv_obj_add_event_cb(app_scr, handle_app_screen_keys_cb, 
                            LV_EVENT_KEY,NULL);
        lv_obj_move_foreground(app_scr);
        
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, app_scr);
        lv_anim_set_time(&a, ANIMATION_SPEED_MS);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
        lv_anim_set_values(&a, UI_CONTAINER_HEIGHT_PX, 0);
        lv_anim_start(&a);
        return app_scr;
    }
}

static void stop_bubble_cb(lv_event_t * e) {
    uint32_t key = lv_event_get_key(e);

    if(key == LV_KEY_LEFT  || key == LV_KEY_RIGHT ||
       key == LV_KEY_UP    || key == LV_KEY_DOWN  ||
       key == LV_KEY_ENTER)
    {
        lv_event_stop_bubbling(e);
    }
}

void draw_app_manager_ui(lv_obj_t* root, application_t** apps, uint8_t num_apps) {
    int total_rows = (num_apps + 1) / GRID_COLUMNS_PER_ROW;
    row_dsc = lv_malloc(sizeof(lv_coord_t) * (total_rows + 1));
    for(int i = 0; i < total_rows; i++) { row_dsc[i] = GRID_ROW_HEIGHT_PX; }
    row_dsc[total_rows] = LV_GRID_TEMPLATE_LAST;

    app_root_cont = lv_obj_create(root);
    remove_shadow_and_outline(app_root_cont);
    lv_obj_add_flag(app_root_cont, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(app_root_cont, UI_CONTAINER_WIDTH_PX, UI_CONTAINER_HEIGHT_PX);
    lv_obj_set_style_bg_color(app_root_cont, COLOR_THEME_SECONDARY, 0);
    lv_obj_set_style_bg_opa(app_root_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(app_root_cont, STYLE_BORDER_WIDTH_DEFAULT, 0);
    lv_obj_set_style_pad_all(app_root_cont, STYLE_PADDING_NONE, 0);
    lv_obj_set_scrollbar_mode(app_root_cont, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *cont = lv_obj_create(app_root_cont);
    lv_obj_set_size(cont, UI_CONTAINER_WIDTH_PX, UI_CONTAINER_HEIGHT_PX);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);
    lv_obj_set_style_bg_opa(cont, STYLE_PADDING_NONE, 0); 
    lv_obj_set_style_border_width(cont, STYLE_BORDER_WIDTH_DEFAULT, 0);
    lv_obj_set_style_pad_all(cont, STYLE_PADDING_NONE, 0);
    lv_obj_set_style_pad_column(cont, STYLE_PADDING_NONE, 0);
    lv_obj_set_style_pad_row(cont, STYLE_PADDING_NONE, 0);
    
    lv_obj_set_style_outline_width(cont, STYLE_OUTLINE_WIDTH_NONE, LV_STATE_ANY);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

    lv_obj_add_event_cb(cont, stop_bubble_cb, LV_EVENT_KEY, NULL);
    make_obj_navigable(cont);
    lv_group_focus_obj(app_root_cont);
    lv_gridnav_add(cont, LV_GRIDNAV_CTRL_ROLLOVER); 

    for(int i = 0; i < num_apps; i++) {
        uint8_t col = i % GRID_COLUMNS_PER_ROW;
        uint8_t row = i / GRID_COLUMNS_PER_ROW;

        lv_obj_t* btn = lv_obj_create(cont);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, col, 1, 
                LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn, open_app, LV_EVENT_KEY, apps[i]);
        
        lv_obj_set_style_bg_color(btn, COLOR_THEME_SECONDARY, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn, STYLE_PADDING_NONE, 0); 
        lv_obj_set_style_border_width(btn, STYLE_BORDER_WIDTH_CELL, 0);
        lv_obj_set_style_border_color(btn, COLOR_THEME_PRIMARY, 0); 
        lv_obj_set_style_pad_all(btn, STYLE_PADDING_NONE, 0);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, 
                LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_set_style_outline_width(btn, STYLE_OUTLINE_WIDTH_NONE, LV_STATE_ANY);
        lv_obj_set_style_bg_color(btn, COLOR_THEME_FOCUS_BG, LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(btn, STYLE_BORDER_WIDTH_FOCUSED, LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(btn, COLOR_THEME_ACCENT, LV_STATE_FOCUSED);
        lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_FULL, LV_STATE_FOCUSED);

        lv_obj_t *icon = lv_image_create(btn);
        lv_image_set_src(icon, apps[i]->ico);
        lv_obj_set_size(icon, UI_ICON_DIMENSION_PX, UI_ICON_DIMENSION_PX);
        
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, apps[i]->name);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(label, COLOR_THEME_TEXT_PRIMARY, 0);
        lv_obj_set_width(label, UI_LABEL_WIDTH_PX);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    }
    lv_obj_update_layout(cont);
}

void del_app_manager_ui(lv_obj_t* root) {
    LOG_INF("Freeing the row_dsc object");
    lv_free(row_dsc);
    app_root_cont = NULL;
}
