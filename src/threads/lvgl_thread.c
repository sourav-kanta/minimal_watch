#include <lvgl.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include "threads/lvgl_thread.h"
#include "managers/ui_manager.h"

#define LVGL_THREAD_STACK_SIZE 8192
#define LVGL_THREAD_PRIORITY 10

LOG_MODULE_REGISTER(lvgl_thread, LOG_LEVEL_INF);

/**
 * @brief Thread ID of LVGL thread
 */
static k_tid_t lvgl_tid = NULL;
/**
 * @brief Control flag for the LVGL thread. Using atomic
 * type so that there are no race conditions
 *
 * @param 0 initialize with 0
 *
 * @return 
 */
atomic_t lvgl_run_flag = ATOMIC_INIT(0);

/**
 * @brief Define the LVGL Stack
 *
 * @param lvgl_thread_stack Name of the stack to be created
 * @param LVGL_THREAD_STACK_SIZE 4KB stack for LVGL
 */
K_THREAD_STACK_DEFINE(lvgl_thread_stack, 
                      LVGL_THREAD_STACK_SIZE);

/**
 * @brief LVGL thread data
 */
struct k_thread lvgl_thread_data;

/**
 * @brief Main thread that calls the lv_timer_handler
 * We are calling this every 25ms since we are expecting 
 * around 20-25fps (max fps will not be > 40) 
 * Here we enter background mode whenever display is inactive
 * for 5 secs. 
 *
 * @param dummy1 Dummy 
 * @param dummy2 Dummy 
 * @param dummy3 Dummy
 */
void lvgl_thread(void *dummy1, void *dummy2, void *dummy3)
{
    // Trigger a user activity to reset inactive time
    // This is needed since on wakeup of device from
    // background mode the lvgl idle timer is stale
    lv_display_trigger_activity(NULL);
    int64_t time_prev, time_now = k_uptime_get();
    while (atomic_get(&lvgl_run_flag) == 1) {
        time_prev = time_now;
        time_now = k_uptime_get();
        int64_t elapsed = time_now - time_prev;
        // Tell LVGL how much time has passed
        lv_tick_inc((uint32_t) elapsed);

        uint32_t lvgl_next_refresh_interval = lv_timer_handler();
        uint32_t idle_time = 
            lv_display_get_inactive_time(NULL);
        //if(idle_time > MAX_USER_INPUT_TIMEOUT) {
        //    atomic_set(&lvgl_run_flag, 0);
        //    // Enter background mode 
        //    deinit_ui();
        //    break;
        //}
        // Sleep for a short period
        //k_msleep(lvgl_next_refresh_interval);
        k_sleep(K_MSEC(30));
    }
}

void init_lvgl_thread() {
    
    if(lvgl_tid) {
        LOG_ERR("LVGL thread already exists");
        return;
    }

    atomic_set(&lvgl_run_flag, 1);
    lvgl_tid = 
        k_thread_create(&lvgl_thread_data, lvgl_thread_stack,
        K_THREAD_STACK_SIZEOF(lvgl_thread_stack),
        lvgl_thread, NULL, NULL, NULL,
        LVGL_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(lvgl_tid, "LVGL Timer");
}

void stop_lvgl_thread() {
    atomic_set(&lvgl_run_flag, 0);
    k_msleep(50);
    lvgl_tid = NULL;
}
