#include "runtime_manager.h"
#include "runtime_management/worker_pool.h"
#include "runtime_management/worker_mgmt.h"
#include "runtime_management/watchdog.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(runtime_mgr, LOG_LEVEL_INF);

K_THREAD_STACK_DEFINE(sys_maint_stack, 2048);
static struct k_thread sys_maint_thread_data;

K_MSGQ_DEFINE(sys_maint_msgq, sizeof(struct runtime_work_item), 12, 4); 

static runtime_state_t current_state = STATE_SLEEP;
static struct k_spinlock state_lock;

static struct k_timer total_work_window_timer;
static struct k_timer grace_period_timer;

static curfew_hook_t active_window_hooks[MAX_CURFEW_HOOKS];
static uint8_t active_hook_count = 0;

static void sys_maint_thread_entry(void *p1, void *p2, void *p3);

static void curfew_hook_async_adapter(void *arg1, atomic_t *arg2)
{
    curfew_hook_t hook = (curfew_hook_t)arg1;
    if (hook != NULL) {
        hook();
    }
}

int add_user_work(struct runtime_work_item *item)
{
    if (item == NULL || item->handler == NULL) return -EINVAL;
    
    if (runtime_get_state() == STATE_SLEEP) {
        return -EACCES;
    }
    return worker_pool_enqueue_user_item(item);
}

int add_system_work(struct runtime_work_item *item)
{
    if (item == NULL || item->handler == NULL) return -EINVAL;
    item->type = WORK_TYPE_SYSTEM;
    return k_msgq_put(&sys_maint_msgq, item, K_NO_WAIT);
}

runtime_state_t runtime_get_state(void)
{
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    runtime_state_t copy = current_state;
    k_spin_unlock(&state_lock, key);
    return copy;
}

uint32_t runtime_manager_get_maint_queue_depth(void)
{
    return k_msgq_num_used_get(&sys_maint_msgq);
}

void set_runtime_state_with_hooks(runtime_state_t target_state, 
        curfew_hook_t hooks[], uint8_t hook_count) {
    k_spinlock_key_t key = k_spin_lock(&state_lock);

    if (current_state == target_state) {
        k_spin_unlock(&state_lock, key);
        return;
    }

    if (target_state == STATE_BACKGROUND_ACTIVE || target_state == STATE_UI_ACTIVE) {
        current_state = target_state;
        worker_mgmt_signal_window_start(k_uptime_get());
        
        worker_pool_resume_all(); 

        int64_t duration = (target_state == STATE_UI_ACTIVE) ? 
                            WINDOW_UI_MAX_MS : WINDOW_BACKGROUND_MAX_MS;
        k_timer_start(&total_work_window_timer, K_MSEC(duration), K_FOREVER);
        
        active_hook_count = (hook_count > MAX_CURFEW_HOOKS) ? MAX_CURFEW_HOOKS : hook_count;
        for (uint8_t i = 0; i < active_hook_count; i++) {
            active_window_hooks[i] = hooks[i];
        }
    } 
    else if (target_state == STATE_SLEEP) {
        k_timer_stop(&total_work_window_timer);
        k_timer_stop(&grace_period_timer);
        worker_mgmt_stop_timers();
        
        watchdog_force_all_cooperative_abort();
        worker_pool_suspend_all(); 
        worker_pool_purge_queue(); 
        
        current_state = STATE_SLEEP;
        LOG_DBG("[FSM] Complete system drop down to zero overhead sleep mode.");
    }

    k_spin_unlock(&state_lock, key);
}

static void execute_curfew_transition_locked(void)
{
    k_timer_stop(&total_work_window_timer);
    worker_mgmt_stop_timers();
    
    if (active_hook_count == 0 && !worker_pool_has_work()) {
        current_state = STATE_SLEEP;
        worker_pool_suspend_all();
        LOG_DBG("[FSM] Nominal window expired with no loads pending. Sleep engaged.");
        return;
    }

    current_state = STATE_GRACE_PERIOD;
    LOG_DBG("[FSM] Window limit reached. Moving to Grace Period. Workers allowed to finish tasks.");
    
    k_timer_start(&grace_period_timer, K_MSEC(WINDOW_GRACE_PERIOD_MS), K_FOREVER);

    for (uint8_t i = 0; i < active_hook_count; i++) {
        struct runtime_work_item hook_item = {
            .type = WORK_TYPE_SYSTEM,
            .priority = BASELINE_PRIORITY,
            .handler = curfew_hook_async_adapter,
            .arg1 = (void*)active_window_hooks[i]
        };
        k_msgq_put(&sys_maint_msgq, &hook_item, K_NO_WAIT);
    }
    active_hook_count = 0;
}

void runtime_manager_demote_to_curfew(void)
{
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    if (current_state == STATE_BACKGROUND_ACTIVE || current_state == STATE_UI_ACTIVE) {
        execute_curfew_transition_locked();
    }
    k_spin_unlock(&state_lock, key);
}

static void total_work_window_expiry(struct k_timer *timer_id)
{
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    if (current_state == STATE_BACKGROUND_ACTIVE || current_state == STATE_UI_ACTIVE) {
        LOG_WRN("[FSM] Core runtime window hardware timeout expired.");
        execute_curfew_transition_locked();
    }
    k_spin_unlock(&state_lock, key);
}

static void grace_period_expiry(struct k_timer *timer_id)
{
    k_spinlock_key_t key = k_spin_lock(&state_lock);
    if (current_state == STATE_GRACE_PERIOD) {
        LOG_ERR("[FSM] Grace period expired! Enforcing hard system shutdown.");
        k_timer_stop(&grace_period_timer);
        
        watchdog_force_all_cooperative_abort();
        worker_pool_suspend_all();
        worker_pool_purge_queue();
        
        current_state = STATE_SLEEP;
    }
    k_spin_unlock(&state_lock, key);
}

static void sys_maint_thread_entry(void *p1, void *p2, void *p3)
{
    struct runtime_work_item item;
    while (1) {
        int status = k_msgq_get(&sys_maint_msgq, &item, K_FOREVER);
        if (status == 0) {
            item.handler(item.arg1, NULL);
            
            if (k_msgq_num_used_get(&sys_maint_msgq) == 0) {
                k_spinlock_key_t key = k_spin_lock(&state_lock);
                if (current_state == STATE_GRACE_PERIOD && !worker_pool_has_work()) {
                    k_timer_stop(&grace_period_timer);
                    watchdog_force_all_cooperative_abort();
                    worker_pool_suspend_all();
                    current_state = STATE_SLEEP;
                    LOG_DBG("[FSM] Maintenance and active tasks fully processed. Entering sleep.");
                }
                k_spin_unlock(&state_lock, key);
            }
            
            runtime_state_t check_st = runtime_get_state();
            if (check_st == STATE_BACKGROUND_ACTIVE || check_st == STATE_UI_ACTIVE) {
                runtime_manager_evaluate_early_curfew();
            }
        }
    }
}

void runtime_manager_init(void)
{
    k_timer_init(&total_work_window_timer, total_work_window_expiry, NULL);
    k_timer_init(&grace_period_timer, grace_period_expiry, NULL);

    watchdog_manager_init();
    worker_pool_init();
    worker_mgmt_init();

    k_thread_create(&sys_maint_thread_data, sys_maint_stack, 2048,
                    sys_maint_thread_entry, NULL, NULL, NULL,
                    BASELINE_PRIORITY - 1, 0, K_NO_WAIT);
    k_thread_name_set(&sys_maint_thread_data, "sys_maintenance");
}
