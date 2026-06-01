#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>
#include <lvgl.h>

LOG_MODULE_REGISTER(encoder, LOG_LEVEL_INF);

static uint32_t pending_key = 0;
static bool key_pending = false;

void keypad_read_callback(lv_indev_t * indev,
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

#ifdef CONFIG_SOC_ESP32S3 
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
#endif

#ifdef CONFIG_ARCH_POSIX
    if(evt->type == INPUT_EV_KEY &&
       evt->value) {
        switch(evt->code) {
            case INPUT_KEY_LEFT :
                pending_key = LV_KEY_LEFT;
                key_pending = true;
                LOG_INF("LV_KEY_LEFT");
                break;
            case INPUT_KEY_RIGHT :
                pending_key = LV_KEY_RIGHT;
                key_pending = true;
                LOG_INF("LV_KEY_RIGHT");
                break;
        }
    }
#endif 

    if(evt->type == INPUT_EV_KEY)
    {
        if(evt->value) {
            switch(evt->code) {
                case INPUT_KEY_ENTER :
                    pending_key = LV_KEY_ENTER;
                    key_pending = true;

                    LOG_INF("LV_KEY_ENTER");
                    break;
                case INPUT_KEY_BACK :
                    pending_key = LV_KEY_ESC;
                    key_pending = true;

                    LOG_INF("LV_KEY_BACK");
                    break;
            }
        }
    }
}

INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);
