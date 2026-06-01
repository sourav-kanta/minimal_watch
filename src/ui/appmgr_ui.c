#include <zephyr/logging/log.h>
#include "managers/ui_manager.h"
#include "ui/appmgr_ui.h"
#include <managers/app_manager.h>
#include "ui/util.h"
#include "managers/input_dev_manager.h"

LOG_MODULE_REGISTER(appmgr_ui, LOG_LEVEL_INF);

static lv_coord_t col_dsc[] = {LV_GRID_FR(1),
                               LV_GRID_FR(1),
                               LV_GRID_TEMPLATE_LAST};
lv_coord_t *row_dsc;

static lv_obj_t* app_root_cont = NULL;
static lv_obj_t* app_scr = NULL;

/**
 * @brief Closes the current app 
 */
void handle_app_scr_close() {
    LOG_INF("Need to close current app");
    if(app_scr == NULL) return;
    close_curr_app();
    lv_obj_delete(app_scr);
    app_scr = NULL;
}

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
                handle_app_scr_close();
                break;
            default:
                LOG_INF("Discarding event as user in app");
                break;
        }
        lv_indev_wait_release(inp_dev);
    }
}

/**
 * @brief Callback for any key presses on the current app
 * The key should be consumed here and should not bubble back
 * to the home screen 
 *
 * @param e
 */
static void handle_app_screen_keys_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e); // Get the pressed key
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
        make_obj_navigable(app_scr);
        lv_group_focus_obj(app_scr);
        lv_obj_add_flag(app_scr, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_size(app_scr, lv_pct(100), lv_pct(100));
        lv_obj_add_event_cb(app_scr, handle_app_scr_gestures,
                            LV_EVENT_GESTURE , NULL);
        lv_obj_add_event_cb(app_scr, handle_app_screen_keys_cb, 
                            LV_EVENT_KEY,NULL);
        lv_obj_move_foreground(app_scr);
        return app_scr;
    }
}

/**
 * @brief Stop event from bubbling back to the home screen 
 *
 * @param e Keypress event 
 */
static void stop_bubble_cb(lv_event_t * e) {
    uint32_t key = lv_event_get_key(e);

    /* If it's a navigation key, stop it from bubbling up to the TabView */
    if(key == LV_KEY_LEFT  || key == LV_KEY_RIGHT ||
       key == LV_KEY_UP    || key == LV_KEY_DOWN  ||
       key == LV_KEY_ENTER)
    {
        lv_event_stop_bubbling(e);
    }
}

/**
 * @brief Draws the app selection UI
 *
 * @param root Root screen
 * @param apps List of all applications
 * @param num_apps Total number of applications
 */
void draw_app_manager_ui(lv_obj_t* root, application_t** apps, uint8_t num_apps) {
    // --- REFINED TACTICAL THEME COLORS ---
    lv_color_t c_grad_top   = lv_color_hex(0x41704e); 
    lv_color_t c_grad_bot   = lv_color_hex(0x0A140F); 
    lv_color_t c_panel      = lv_color_hex(0x1B3125); 
    lv_color_t c_focus_bg   = lv_color_hex(0x2D523E); // Deep forest green
    lv_color_t c_tactical   = lv_color_hex(0x50C878); // Muted Emerald
    lv_color_t c_white      = lv_color_hex(0xE0E0E0); 

    // 2. Row Descriptor
    int total_rows = (num_apps + 1) / 2;
    row_dsc = lv_malloc(sizeof(lv_coord_t) * (total_rows + 1));
    for(int i = 0; i < total_rows; i++) { row_dsc[i] = 80; }
    row_dsc[total_rows] = LV_GRID_TEMPLATE_LAST;

    // 3. Wrapper Container
    app_root_cont = lv_obj_create(root);
    remove_shadow_and_outline(app_root_cont);
    lv_obj_add_flag(app_root_cont, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_size(app_root_cont, 128, 160);
    lv_obj_set_style_bg_color(app_root_cont, c_panel, 0);
    lv_obj_set_style_bg_opa(app_root_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(app_root_cont, 0, 0);
    lv_obj_set_style_pad_all(app_root_cont, 0, 0);
    lv_obj_set_scrollbar_mode(app_root_cont, LV_SCROLLBAR_MODE_OFF);

    // 4. Grid Container
    lv_obj_t *cont = lv_obj_create(app_root_cont);
    lv_obj_set_size(cont, 128, 160);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);
    lv_obj_set_style_bg_opa(cont, 0, 0); 
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_pad_column(cont, 0, 0);
    lv_obj_set_style_pad_row(cont, 0, 0);
    
    lv_obj_set_style_outline_width(cont, 0, LV_STATE_ANY);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

    lv_obj_add_event_cb(cont, stop_bubble_cb, LV_EVENT_KEY, NULL);
    make_obj_navigable(cont);
    lv_gridnav_add(cont, LV_GRIDNAV_CTRL_ROLLOVER); 

    for(int i = 0; i < num_apps; i++) {
        uint8_t col = i % 2;
        uint8_t row = i / 2;

        lv_obj_t* btn = lv_obj_create(cont);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, col, 1, 
                LV_GRID_ALIGN_STRETCH, row, 1);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn, open_app, LV_EVENT_CLICKED, apps[i]);
        
        // --- BUTTON BASE: Flat & Integrated ---
        lv_obj_set_style_bg_color(btn, c_panel, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn, 0, 0); 
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_border_color(btn, c_grad_bot, 0); 
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, 
                LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // --- TACTICAL FOCUS (FULL BOX) ---
        lv_obj_set_style_outline_width(btn, 0, LV_STATE_ANY);
        lv_obj_set_style_bg_color(btn, c_focus_bg, LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(btn, 2, LV_STATE_FOCUSED);
        lv_obj_set_style_border_color(btn, c_tactical, LV_STATE_FOCUSED);
        lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_FULL, LV_STATE_FOCUSED);

        // Icon
        lv_obj_t *icon = lv_image_create(btn);
        lv_image_set_src(icon, apps[i]->ico);
        lv_obj_set_size(icon, 32, 32);
        
        // Label
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, apps[i]->name);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(label, c_white, 0);
        lv_obj_set_width(label, 60);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
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

