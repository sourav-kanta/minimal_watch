#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>

#include "display/lv_display.h"
#include "managers/ui_manager.h"
#include "managers/app_manager.h"
#include "managers/timers.h"
#include "threads/lvgl_thread.h"
#include "ui/watchface/wf_picker.h"
#include "ui/appmgr_ui.h"
#include "ui/util.h"

static lv_obj_t *root_screen;
static lv_obj_t *home_tab, *wf_page, *notify_page, *app_page;
static watchface_t *selected_wf;
static ui_state_t curr_watch_state = _START;
static display_state_t dState = DIS_INVALID;
static bool is_first_render = true;

LOG_MODULE_REGISTER(ui, LOG_LEVEL_INF);


/**
 * @brief Switch on the UI and update state
 *
 * @param display_dev Display device
 */
void turn_on_Ui(const struct device* display_dev) {
    int ret = display_blanking_off(
            display_dev);
    if(ret < 0 || ret == -ENOSYS)
        LOG_ERR("Cant find display driver");
    __ASSERT_NO_MSG(ret >= 0 && ret != -ENOSYS);
    dState = ON;
    LOG_INF("Device blanking turned off.");
}

/**
 * @brief Switch off the device and update state
 *
 * @param display_dev Display device
 */
void turn_off_Ui(const struct device* display_dev) {
    int ret = display_blanking_on(display_dev);
    if(ret < 0 || ret == -ENOSYS)
        LOG_ERR("Cant find display driver");
    __ASSERT_NO_MSG(ret >= 0 && ret != -ENOSYS);
    dState = OFF;
    LOG_INF("Device blanking turned on.");
}

/**
 * @brief Unimplemented version of updating the UI
 * Needs to be refined further to handle all updates
 *
 * @todo Replace implementation with get_time()
 */
void forward_wf_event(event_t *event) {
    switch(event->ev) {
        case EVENT_TICK_UPDATE:
            if(selected_wf != NULL) {
                LOG_INF("1s tick");
                selected_wf->update_watchface(event);
            }
            break;
    }
}

/**
 * @brief Closes the current page by calling all the 
 * delete function pointers
 */
void close_curr_page() {
    ui_state_t page = get_current_ui_state();
    switch(page) {
        case WATCHFACE :
            // If its first render this call is invalid
            if(!is_first_render) {
                stop_specific_timer(WF_1S_TIMER);
            }
            lv_obj_clean(wf_page);
            selected_wf = NULL;
            LOG_INF("Deleting Watchface UI");
            break;
        case APP :
            LOG_INF("Deleting App manager UI");
            lv_obj_clean(app_page);
            del_app_manager_ui(app_page);
            break;
        default :
            LOG_ERR("Unknown page to close");
            break;
    }

}

/**
 * @brief  Handles displaying a specific page
 *
 * @param page Page to be displayed
 */
void show_page(ui_state_t page) {
    switch(page) {
        case WATCHFACE :
            start_specific_timer(WF_1S_TIMER);
            selected_wf = select_wf();
            LOG_INF("Selected watchface : %s",
                    selected_wf->name);
            selected_wf->draw_watchface(wf_page);
            break;
        case APP :
            show_app_picker_ui(app_page);
            break;
        default :
            LOG_ERR("Unknown page to show");
            break;
    }

}

/**
 * @brief Root screen navigation handling for
 * right key or gesture 
 */
void handle_right_action_on_root() {
    if(curr_watch_state !=(_END -1)) {
        close_curr_page();
        curr_watch_state++;
        uint32_t act = lv_tabview_get_tab_active(home_tab);
        lv_tabview_set_active(home_tab, act + 1, LV_ANIM_ON);
        show_page(curr_watch_state);
    }
}

/**
 * @brief Root screen navigation for left key 
 * gesture or key 
 */
void handle_left_action_on_root() {
    if(curr_watch_state !=(_START +1)) {
        close_curr_page();
        curr_watch_state--;
        uint32_t act = lv_tabview_get_tab_active(home_tab);
        lv_tabview_set_active(home_tab, act - 1, LV_ANIM_ON);
        show_page(curr_watch_state);
    }
}


/**
 * @brief Handles root screen gestures
 * @todo Change watch base page with gestures
 *
 * @param ev LVGL event received on callback
 */
void handle_root_scr_actions(lv_event_t *ev) {
    lv_event_code_t code = lv_event_get_code(ev);
    lv_indev_t *inp_dev = lv_indev_active();
    lv_dir_t ges_dir = LV_DIR_NONE;
    uint32_t key = LV_KEY_HOME; // None key 
    if(code == LV_EVENT_GESTURE) {
        ges_dir = lv_indev_get_gesture_dir(inp_dev);
    }

    if(code == LV_EVENT_KEY) {
        key = lv_event_get_key(ev);
    // Skip the fake key 0 SDL sends  
#ifdef CONFIG_ARCH_POSIX
        if(key == 0) return;
#endif
    }

    // Child should have stopped bubbling if it had consumed
    // the key, since it didnt the root should handle it
    if(ges_dir == LV_DIR_RIGHT || key == LV_KEY_LEFT) {
        LOG_INF("Left action");
        handle_left_action_on_root();
    }
    if(ges_dir == LV_DIR_LEFT || key == LV_KEY_RIGHT) {
        LOG_INF("Right action");
        handle_right_action_on_root();
    }
    // BFS forwards to get any focusable children
    if(key == LV_KEY_ENTER) {
        LOG_INF("Ok button");
        lv_event_stop_bubbling(ev);
        lv_indev_wait_release(inp_dev);
        lv_obj_t * focused = lv_group_get_focused(lv_group_get_default());
        if(focused == NULL) return;
        lv_obj_t* target = find_first_focusable_bfs(focused);
        if(target) {
            LOG_INF("BFS found nearest focusable child");
            lv_group_focus_obj(target);
            return;
        }

    }
    // DFS backwards to get navigable parent
    if(key == LV_KEY_ESC) {
        LOG_INF("Back button");
        lv_obj_t * focused = lv_group_get_focused(lv_group_get_default());
        if(focused == NULL) return;
        lv_obj_t * parent = lv_obj_get_parent(focused);
        while(parent) {
            if(lv_obj_get_group(parent) == lv_group_get_default()) {
                lv_group_focus_obj(parent);
                // Stop LVGL from processing this key further
                lv_indev_wait_release(inp_dev);
                lv_event_stop_bubbling(ev);
                return;
            }
            parent = lv_obj_get_parent(parent);
        }
    }
    lv_indev_wait_release(inp_dev);
}


/**
 * @brief Initialize the UI from the device tree
 * Log current LVGL version
 * Set the root screen for children to use
 * If starting from boot set the default watchface
 * A quirk is that since zephyr handles the LVGL 
 * task handler thread, we need to block sleep for 
 * some time so that native posix is happy to draw
 * otherwise it will not redraw a bit of the screen
 * as LVGL thinks its has aldready drawn it on screen
 */
void init_ui() {    
    if(dState == ON) {
        LOG_ERR("Display is already on");
        return;
    }

    const struct device *display_dev;
    display_dev = DEVICE_DT_GET(
            DT_CHOSEN(zephyr_display));
    __ASSERT_NO_MSG(device_is_ready(display_dev));
    lv_display_set_antialiasing(lv_display_get_default(), false);    
    LOG_INF("Device driver is setup up");
    
    turn_on_Ui(display_dev);
    init_lvgl_thread();
    
    LOG_INF("LVGL Version: %d.%d.%d", 
            LVGL_VERSION_MAJOR,
            LVGL_VERSION_MINOR,
            LVGL_VERSION_PATCH);
    root_screen = lv_scr_act(); 
    home_tab = lv_tabview_create(root_screen);
    // Hide the tab_bar
    lv_tabview_set_tab_bar_size(home_tab, 0);
    lv_obj_t * tab_bar = lv_tabview_get_tab_bar(home_tab);
    lv_obj_remove_flag(tab_bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(tab_bar, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    //Make the table view respond to keys
    lv_obj_t* tabview_content = lv_tabview_get_content(home_tab);
    lv_obj_add_flag(tabview_content, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_remove_flag(tabview_content, LV_OBJ_FLAG_SCROLLABLE);
    // Add the pages and make them navigable
    wf_page = lv_tabview_add_tab(home_tab, "Watchface");
    remove_shadow_and_outline(wf_page);
    app_page = lv_tabview_add_tab(home_tab, "Apps");
    remove_shadow_and_outline(app_page);
    notify_page = lv_tabview_add_tab(home_tab, "Notifications");
    remove_shadow_and_outline(notify_page);
    lv_obj_add_event_cb(tabview_content, 
                        handle_root_scr_actions,
                        LV_EVENT_GESTURE,
                        NULL);
    lv_obj_add_event_cb(tabview_content, 
                        handle_root_scr_actions,
                        LV_EVENT_KEY,
                        NULL);
    make_obj_navigable(tabview_content);
    lv_obj_add_flag(app_page, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(wf_page, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(notify_page, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_group_focus_obj(tabview_content); 
    k_msleep(100);
    curr_watch_state = WATCHFACE;
    show_page(curr_watch_state);
    is_first_render = false;
}

/**
 * @brief Prepares the UI to be shot down 
 */
void deinit_ui() {
    
    if(dState == DIS_INVALID || dState == OFF) {
        LOG_ERR("Display is already off");
        return;
    }

    close_curr_page();
    is_first_render = true;
    const struct device *display_dev;
    display_dev = DEVICE_DT_GET(
            DT_CHOSEN(zephyr_display));
    stop_lvgl_thread();
    turn_off_Ui(display_dev);
    curr_watch_state = _START; 
}

/**
 * @brief Returns the watches current UI state
 *
 * @return current ui_state_t 
 */
ui_state_t get_current_ui_state() {
    return curr_watch_state;
}

display_state_t get_current_display_state() {
    return dState;
}

/**
 * @brief Get the lowest level root root_screen 
 *
 * @return lv_obj_t of base object
 */
lv_obj_t* get_root_screen() {
    return root_screen;
}

watchface_t* get_selected_wf() {
    return selected_wf;
}
