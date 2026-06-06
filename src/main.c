/**
 * @mainpage A smartwatch firmware inspired by ZSWatch
 *
 * @section intro Introduction
 * Smartwatch firmware using Zephyr RTOS, ultimate goal
 * of acheiving a working feature rich smartwatch
 * 
**/  

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "managers/ui_manager.h"
#include "managers/gpio_manager.h"
#include "managers/ble_manager.h"
#include "managers/input_dev_manager.h"
#include "managers/pm_manager.h"
#include "managers/event_manager.h"
#include "managers/persistance_manager.h"
#include "managers/runtime_manager.h"
#include "managers/timers.h"
#include "managers/storage_manager.h"
#include "managers/notification_manager.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/**
 * @brief Init work on startup
 */
void do_init_work() {
    init_persistance();
    init_gpio_pins();
    init_ble();
    setup_keyboard();
    runtime_manager_init();
    notification_init();    
    init_ui();
    init_pm();
    init_events();
}

int main(void)
{
    do_init_work();
    start_specific_timer(WORK_WINDOW_START);
    write_file();
    return 0;
}

