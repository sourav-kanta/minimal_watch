#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>

#include "managers/ui_manager.h"
#include "managers/app_manager.h"
#include "managers/timers.h"
#include "threads/lvgl_thread.h"
#include "ui/watchface/wf_picker.h"
#include "ui/appmgr_ui.h"

static lv_obj_t *root_screen;
static watchface *selected_wf;
static ui_state curr_watch_state = _START;
static display_state dState = DIS_INVALID;
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
 * Runs on a delayable work thread so no worry about block
 *
 * @todo Replace implementation with get_time()
 */
void update_ui(wf_event ev) {
    switch(ev.event_type) {
        case UI_WF_TIMER_UPDATE:
            if(selected_wf != NULL) {
                selected_wf->update_watchface(root_screen,
                        ev);
            }
            break;
    }
}

/**
 * @brief Closes the current page by calling all the 
 * delete function pointers
 */
void close_curr_page() {
    ui_state page = get_current_ui_state();
    switch(page) {
        case WATCHFACE :
            // If its first render this call is invalid
            if(!is_first_render) {
                stop_specific_timer(UI_WF_TIMER_UPDATE);
            }
            lv_obj_clean(root_screen);
            LOG_INF("Deleting Watchface UI");
            break;
        case APP :
            LOG_INF("Deleting App manager UI");
            lv_obj_clean(root_screen);
            del_app_manager_ui(root_screen);
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
void show_page(ui_state page) {
    switch(page) {
        case WATCHFACE :
            start_specific_timer(UI_WF_TIMER_UPDATE);
            selected_wf = select_wf();
            LOG_INF("Selected watchface : %s",
                    selected_wf->name);
            selected_wf->draw_watchface(root_screen);
            break;
        case APP :
            show_app_picker_ui(root_screen);
            break;
        default :
            LOG_ERR("Unknown page to show");
            break;
    }

}

/**
 * @brief Handles root screen gestures
 * @todo Change watch base page with gestures
 *
 * @param ev LVGL event received on callback
 */
void handle_root_scr_gestures(lv_event_t *ev) {
    lv_event_code_t code = lv_event_get_code(ev);
    lv_indev_t *inp_dev = lv_indev_active();
    if(code == LV_EVENT_GESTURE) {
        lv_dir_t ges_dir = lv_indev_get_gesture_dir(
                                inp_dev);
        switch(ges_dir) {
            case LV_DIR_TOP :
                LOG_INF("Up gesture detected");
                break;
            case LV_DIR_BOTTOM :
                LOG_INF("Down gesture detected");
                break;
            case LV_DIR_LEFT :
                LOG_INF("Left gesture detected");
                if(curr_watch_state !=(_END -1)) {
                    close_curr_page();
                    curr_watch_state++;
                    show_page(curr_watch_state);
                }
                break;
            case LV_DIR_RIGHT :
                LOG_INF("Right gesture detected");
                if(curr_watch_state !=(_START +1)) {
                    close_curr_page();
                    curr_watch_state--;
                    show_page(curr_watch_state);
                }
                break;
            default:
                break;
        }
        lv_indev_wait_release(inp_dev);
    }
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
    
    LOG_INF("Device driver is setup up");
    
    turn_on_Ui(display_dev);
    init_lvgl_thread();
    
    LOG_INF("LVGL Version: %d.%d.%d", 
            LVGL_VERSION_MAJOR,
            LVGL_VERSION_MINOR,
            LVGL_VERSION_PATCH);
    root_screen = lv_scr_act();
    
    // Add gesture control to switch pages
    lv_obj_add_event_cb(root_screen, 
                        handle_root_scr_gestures,
                        LV_EVENT_GESTURE,
                        NULL);

    k_msleep(100);
    curr_watch_state = WATCHFACE;
    show_page(curr_watch_state);
    is_first_render = false;
}

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
 * @return 
 */
ui_state get_current_ui_state() {
    return curr_watch_state;
}
