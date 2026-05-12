/**
 * @file pm_manager.c
 * @brief Power management modules
 * @author Sourav Kanta
 * @version v1
 * @date 2026-05-03
 */
#include <zephyr/pm/pm.h>
#include <zephyr/logging/log.h> 

LOG_MODULE_REGISTER(PM, LOG_LEVEL_INF); 

static void on_pm_state_entry(enum pm_state state) {
    switch(state) {
        case PM_STATE_ACTIVE :
            LOG_INF("Waking up : Enter ");
            break;
        case PM_STATE_SUSPEND_TO_IDLE:
            LOG_INF("Sleeping : Exit ");
            break;
        default :
            break; 
    }
}

static void on_pm_state_exit(enum pm_state state) {
    switch(state) {
        case PM_STATE_ACTIVE :
            LOG_INF("Waking : Exit ");
            break;
        case PM_STATE_SUSPEND_TO_IDLE:
            LOG_INF("Sleeping : Exit ");
            break;
        default :
            break;
    } 
}

static struct pm_notifier notify_hooks = {
    .state_entry = on_pm_state_entry,
    .state_exit = on_pm_state_exit,
};


void init_pm() {
    pm_notifier_register(&notify_hooks);    
}


