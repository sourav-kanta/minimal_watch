#include <managers/app_manager.h>
#include "assets/cloudy.h"
#include <common_types.h>

static void draw_app_ui(lv_obj_t* parent_obj) {

}

static void delete_app_ui(lv_obj_t* parent_obj) {

}

static void receive_event(event_t* e) {

}


static application_t weather_app = {
    .name = "Weather",
    .ico = &cloudy,
    .draw_app = draw_app_ui,
    .close_app = delete_app_ui,
    .handle_event = receive_event 
};

static int register_app(void) {
    add_app(&weather_app);
    return 0;
}

SYS_INIT(register_app, APPLICATION, APP_PRIORITY);

