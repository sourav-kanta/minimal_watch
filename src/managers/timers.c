#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "managers/event_manager.h"
#include "managers/timers.h"
#include "common_types.h"


LOG_MODULE_REGISTER(timer, LOG_LEVEL_INF);

/**
 * @brief This is called in IRQ context to update watchface 
 * every second
 *
 * @param work
 */
static void publish_tick_event(struct k_work *work) {
    event_t tick_ev = { .ev = EVENT_TICK_UPDATE, 
                        .payload_len = 0,
                        .data = NULL };
    handle_event(&tick_ev);    
    k_work_reschedule(k_work_delayable_from_work(work),
            K_SECONDS(1));
}

static void publish_work_event(struct k_work *work) {
    k_work_reschedule(k_work_delayable_from_work(work),
            K_SECONDS(1));
    event_t work_ev = { .ev = EVENT_WORK_TICK,
                        .payload_len = 0,
                        .data = NULL };
    handle_event(&work_ev);
}

K_WORK_DELAYABLE_DEFINE(wf_1s_refresh_work, publish_tick_event);
K_WORK_DELAYABLE_DEFINE(work_window_start, publish_work_event);

/**
 * @brief Stops a specific timer
 *
 * @param ttype timer to stop
 */
void stop_specific_timer(mw_timer_t ttype) {
    switch(ttype) {
        case WF_1S_TIMER :
            LOG_INF("Stopping timer WF_1S_TIMER");
            struct k_work_sync sync_obj; 
            k_work_cancel_delayable_sync(&wf_1s_refresh_work,
                                    &sync_obj);
            break;
        case WORK_WINDOW_START :
            LOG_WRN("Trying to stop work tick. Invalid");
            break;
    }
}

/**
 * @brief Starts a specific timer
 *
 * @param ttype timer to stop
 */
void start_specific_timer(mw_timer_t ttype) {
    switch(ttype) {
        case WF_1S_TIMER :
            LOG_INF("Starting timer WF_1S_TIMER");
            if(!k_work_delayable_is_pending(
                        &wf_1s_refresh_work))
                    k_work_reschedule(&wf_1s_refresh_work,
                        K_MSEC(0));
            break;
        case WORK_WINDOW_START :
            LOG_INF("Starting WORK_TIMER");
            if(!k_work_delayable_is_pending(
                        &work_window_start))
                    k_work_reschedule(&work_window_start,
                        K_MSEC(0));

            break;

    }
}

/**
 * @brief Start all the timers 
 */
void start_timers() {
    start_specific_timer(WF_1S_TIMER);
    start_specific_timer(WORK_WINDOW_START);
}


/**
 * @brief Stop all timers
 */
void stop_timers() {
    stop_specific_timer(WF_1S_TIMER);
    stop_specific_timer(WORK_WINDOW_START);
}



