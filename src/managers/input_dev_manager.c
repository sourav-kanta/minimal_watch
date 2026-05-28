#include <zephyr/input/input.h>
#include <lvgl.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(input_dev, LOG_LEVEL_INF);

static uint32_t pending_key = 0;
static bool key_pending = false;

static void keypad_read(lv_indev_t * indev,
                        lv_indev_data_t * data)
{
    if(key_pending) {
        data->key = pending_key;
        data->state = LV_INDEV_STATE_PRESSED;

        key_pending = false;
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void input_cb(struct input_event *evt, void *user_data)
{
    LOG_INF("type=%d code=%d value=%d",
            evt->type,
            evt->code,
            evt->value);

    if(evt->type == INPUT_EV_REL &&
       evt->code == INPUT_REL_WHEEL)
    {
        if(evt->value > 0) {
            pending_key = LV_KEY_RIGHT;
            key_pending = true;

            LOG_INF("LV_KEY_RIGHT");
        }
        else if(evt->value < 0) {
            pending_key = LV_KEY_LEFT;
            key_pending = true;

            LOG_INF("LV_KEY_LEFT");
        }
    }

    if(evt->type == INPUT_EV_KEY &&
       evt->code == INPUT_KEY_ENTER)
    {
        if(evt->value) {
            pending_key = LV_KEY_ENTER;
            key_pending = true;

            LOG_INF("LV_KEY_ENTER");
        }
    }
}

INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

static lv_indev_t * keypad_indev;
static lv_group_t * key_grp;

void setup_keyboard(void)
{
    keypad_indev = lv_indev_create();
    lv_indev_set_type(keypad_indev,
                      LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keypad_indev,
                         keypad_read);

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
