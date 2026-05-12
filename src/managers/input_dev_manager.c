/**
 * @file input_dev_manager.c
 * @brief Input Devices API
 * @author Sourav Kanta
 * @version v1
 * @date 2026-04-10
 */

#include "indev/lv_indev.h"
#include "managers/ui_manager.h"
#include "misc/lv_event.h"
#include "ui/appmgr_ui.h"
#include "managers/input_dev_manager.h"
#include <lvgl.h>
#include <zephyr/logging/log.h>

static lv_indev_t* keypad = NULL;
static lv_group_t* key_grp = NULL;

LOG_MODULE_REGISTER(input_dev, LOG_LEVEL_INF);


/**
 * @brief Setup the input devices
 */
void setup_keyboard() {
    lv_indev_t * indev = NULL;
    while ((indev = lv_indev_get_next(indev))) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD) {
            keypad = indev;
            key_grp = lv_group_create();
            lv_group_set_default(key_grp); 
            lv_indev_set_group(indev, key_grp);
            LOG_INF("Keyboard group added");
        }
    }
}

