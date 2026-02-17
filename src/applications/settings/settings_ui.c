#include <managers/app_manager.h>
#include "assets/setting_ico.h"
#include <common_types.h>

void draw_app_ui(lv_obj_t* parent_obj) {

}

void delete_app_ui(lv_obj_t* parent_obj) {

}

static application settings_app = {
    .name = "Settings",
    .ico = &setting_ico,
    .draw_app = draw_app_ui,
    .close_app = delete_app_ui,
};

static int register_app(void) {
    add_app(&settings_app);
    return 0;
}

SYS_INIT(register_app, APPLICATION, APP_PRIORITY);
