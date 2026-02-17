#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include "ui/appmgr_ui.h"

LOG_MODULE_REGISTER(app_manager, LOG_LEVEL_INF);

static application* all_apps[MAX_APPS];
static application* curr_app = NULL;
static uint8_t num_apps = 0;
static bool is_app_running = false;

void add_app(application* app) {
    all_apps[num_apps++] = app;
}

void open_app(lv_event_t* ev) {
    application* app = 
        (application*) lv_event_get_user_data(ev);
    LOG_INF("Opening app %s", app->name);
    curr_app = app;
    is_app_running = true;
    app->draw_app(create_app_screen());
}

void close_curr_app() {
    if(is_app_running == false) {
        LOG_ERR("No running apps");
    }
    else {
        if(curr_app) { 
            curr_app->close_app();
            LOG_INF("Closing app %s", curr_app->name);
        }
        is_app_running = false;
        curr_app = NULL;
    }
}

void show_app_picker_ui(lv_obj_t* root) {
    LOG_INF("Drawing the app manager UI");
    draw_app_manager_ui(root, all_apps, num_apps);
}

