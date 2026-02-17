#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "managers/ui_manager.h"
#include "managers/timers.h"
#include "common_types.h"


LOG_MODULE_REGISTER(timer, LOG_LEVEL_INF);

/**
 * @brief This is called in IRQ context to update watchface 
 * every second, this will create a wf_event that will be
 * consumed in UI manager and made available to final wf app
 *
 * @param work
 */
void send_wf_update_event(struct k_work *work) {
    // Create the event on ZBUS after a check if watchface 
    // is the current application running
    LOG_DBG("Updating the watchface");
    ui_state curr_state = get_current_ui_state();
    if(curr_state != WATCHFACE) { 
        LOG_ERR("No available watchface");
        return;
    }

    wf_event time_ev;
    time_ev.event_type = UI_WF_TIMER_UPDATE;
    int64_t sec = k_uptime_get();
    date_time dat;
    dat.sec = (sec/1000)%60;
    time_ev.data = &dat;
    update_ui(time_ev);
    k_work_reschedule(k_work_delayable_from_work(work),
            K_SECONDS(1));
}

K_WORK_DELAYABLE_DEFINE(wf_1s_refresh_work, send_wf_update_event);

/**
 * @brief Stops a specific timer
 *
 * @param ttype timer to stop
 */
void stop_specific_timer(timer_type ttype) {
    switch(ttype) {
        case WF_1S_TIMER :
            LOG_INF("Stopping timer WF_1S_TIMER");
            struct k_work_sync sync_obj; 
            k_work_cancel_delayable_sync(&wf_1s_refresh_work,
                                    &sync_obj);
            break;
    }
}

/**
 * @brief Starts a specific timer
 *
 * @param ttype timer to stop
 */
void start_specific_timer(timer_type ttype) {
    switch(ttype) {
        case WF_1S_TIMER :
            LOG_INF("Starting timer WF_1S_TIMER");
            if(!k_work_delayable_is_pending(
                        &wf_1s_refresh_work))
                    k_work_reschedule(&wf_1s_refresh_work,
                        K_MSEC(50));
            break;
    }
}

/**
 * @brief Start all the timers 
 */
void start_timers() {
    start_specific_timer(WF_1S_TIMER);
}


/**
 * @brief Stop all timers
 */
void stop_timers() {
    stop_specific_timer(WF_1S_TIMER);
}



