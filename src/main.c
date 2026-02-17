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

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);


/**
 * @brief Init work on startup
 */
void do_init_work() {
    init_ui();
    init_gpio_pins();
}

int main(void)
{
    do_init_work();

    return 0;
}

