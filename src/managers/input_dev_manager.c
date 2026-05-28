#include <lvgl.h>
#include <zephyr/logging/log.h>
#include "HAL/navigation/encoder.h"

LOG_MODULE_REGISTER(input_dev, LOG_LEVEL_INF);

static lv_indev_t * keypad_indev;
static lv_group_t * key_grp;

void setup_keyboard(void)
{
    keypad_indev = lv_indev_create();
    lv_indev_set_type(keypad_indev,
                      LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keypad_indev,
                         keypad_read_callback);

    key_grp = lv_group_create();
    lv_group_set_default(key_grp);
    lv_indev_set_group(keypad_indev,
                       key_grp);

    LOG_INF("Custom keypad indev ready");
}

lv_group_t * get_current_group(void)
{
    return key_grp;
}
