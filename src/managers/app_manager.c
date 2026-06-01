#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include "ui/appmgr_ui.h"
#include "common_utils.h"

LOG_MODULE_REGISTER(app_manager, LOG_LEVEL_INF);

static application_t* all_apps[MAX_APPS];
static application_t* curr_app = NULL;
static uint8_t num_apps = 0;
static bool is_app_running = false;

/**
 * @brief Registers a app with the app manager 
 *
 * @param app App to be registered
 */
void add_app(application_t* app) {
    app->app_id = num_apps;
    all_apps[num_apps++] = app;
}

/**
 * @brief Starts a new app 
 *
 * @param ev Event that triggered the app start
 */
void open_app(lv_event_t* ev) {
    lv_event_stop_bubbling(ev);
    application_t* app = 
        (application_t*) lv_event_get_user_data(ev);
    LOG_INF("Opening app %s", app->name);
    curr_app = app;
    is_app_running = true;
    app->draw_app(create_app_screen());
}

/**
 * @brief Closes the current app
 */
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

/**
 * @brief Draws the app picker UI
 *
 * @param root Base lv_obj on top of whch we draw
 * the app picking UI
 */
void show_app_picker_ui(lv_obj_t* root) {
    LOG_INF("Drawing the app manager UI");
    draw_app_manager_ui(root, all_apps, num_apps);
}

/**
 * @brief Checks if app is currently is_app_running
 *
 * @return true is app is running else false 
 */
bool check_if_app_running() {
    return is_app_running;
}

/**
 * @brief Get the current running app 
 *
 * @return NULL if no running app else reference to 
 * current running application 
 */
application_t* get_curr_app() {
    return curr_app;
}

void send_app_update(app_update_t *update, atomic_t* abort) {
    if(generate_curr_app_id() != update->req_app) {
        LOG_ERR("App no longer in focus. Discarding response");
        return;
    }

    if(!curr_app) {
        LOG_ERR("No application currently active.");
        return;
    }

    if(curr_app->handle_event)
        (*curr_app->handle_event)(update);
}

