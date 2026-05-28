#include "runtime_management/worker_mgmt.h"
#include "runtime_management/worker_pool.h"
#include "managers/runtime_manager.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(worker_mgmt, LOG_LEVEL_INF);

static struct k_timer curfew_settlement_timer; 
static int64_t window_start_time = 0;

static inline bool is_system_completely_idle(void)
{
    return (!worker_pool_has_work() && runtime_manager_get_maint_queue_depth() == 0);
}

static void curfew_settlement_expiry(struct k_timer *timer_id)
{
    runtime_state_t state = runtime_get_state();
    
    if (state == STATE_BACKGROUND_ACTIVE || state == STATE_UI_ACTIVE) {
        if (is_system_completely_idle()) {
            LOG_DBG("[Mgmt] Stabilization cushion dry for 10ms. Relaying early curfew transition.");
            runtime_manager_demote_to_curfew(); 
        }
    } else if (state == STATE_GRACE_PERIOD) {
        if (is_system_completely_idle()) {
            LOG_DBG("[Mgmt] Grace channel fully parsed. Initiating state drop.");
            set_runtime_state_with_hooks(STATE_SLEEP, NULL, 0); 
        }
    }
}

void worker_mgmt_signal_window_start(int64_t uptime)
{
    window_start_time = uptime;
    k_timer_stop(&curfew_settlement_timer);
}

void worker_mgmt_stop_timers(void)
{
    k_timer_stop(&curfew_settlement_timer);
}

void runtime_manager_interrupt_settlement(void)
{
    k_timer_stop(&curfew_settlement_timer);
}

void runtime_manager_evaluate_early_curfew(void)
{
    runtime_state_t state = runtime_get_state();
    
    if (state == STATE_BACKGROUND_ACTIVE || 
            state == STATE_UI_ACTIVE || 
            state == STATE_GRACE_PERIOD) {
        if (is_system_completely_idle()) {
            if (k_timer_remaining_get(&curfew_settlement_timer) > 0) {
                return; 
            }

            int64_t elapsed_now = k_uptime_get() - window_start_time;
            int64_t limit_ms = (state == STATE_UI_ACTIVE) ?
                                 WINDOW_UI_MAX_MS : WINDOW_BACKGROUND_MAX_MS;
            int64_t remaining_to_limit = limit_ms - elapsed_now;
            
            if (remaining_to_limit > 0) {
                int64_t gap_duration = (remaining_to_limit < 10) ? remaining_to_limit : 10;
                k_timer_start(&curfew_settlement_timer, K_MSEC(gap_duration), K_FOREVER);
            }
            else {
                runtime_manager_demote_to_curfew();
            }
        }
    }
}

void worker_mgmt_init(void)
{
    k_timer_init(&curfew_settlement_timer, curfew_settlement_expiry, NULL);
}
