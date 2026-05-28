#include "runtime_management/watchdog.h"
#include "runtime_management/worker_pool.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wd_mgr, LOG_LEVEL_INF);

static struct k_timer user_app_soft_wd[WORKER_POOL_SIZE];
static struct k_timer user_app_hard_wd[WORKER_POOL_SIZE];
static atomic_t user_app_abort_flags[WORKER_POOL_SIZE];

static void user_app_soft_wd_expiry(struct k_timer *timer_id)
{
    int idx = (int)(timer_id - user_app_soft_wd);
    if (idx < 0 || idx >= WORKER_POOL_SIZE) return;

    LOG_WRN("[Watchdog] Slot [%d] crossed 30ms limit. Sending abort signal.", idx);
    atomic_set(&user_app_abort_flags[idx], 1);
    k_timer_start(&user_app_hard_wd[idx], K_MSEC(TIMEOUT_USER_APP_HARD_MS), K_FOREVER);
}

static void user_app_hard_wd_expiry(struct k_timer *timer_id)
{
    int idx = (int)(timer_id - user_app_hard_wd);
    if (idx < 0 || idx >= WORKER_POOL_SIZE) return;

    LOG_ERR("[Watchdog] Hard lock up detected on slot [%d]. Re-initializing.", idx);
    
    k_tid_t target_worker = worker_pool_get_tid(idx);
    
    k_timer_stop(&user_app_soft_wd[idx]);
    k_timer_stop(&user_app_hard_wd[idx]);
    atomic_set(&user_app_abort_flags[idx], 0);

    if (target_worker != NULL) {
        k_thread_abort(target_worker);
    }
    
    worker_pool_recover_stalled_runner(idx);
}

void watchdog_start_user_app(int worker_idx)
{
    if (worker_idx < 0 || worker_idx >= WORKER_POOL_SIZE) return;
    atomic_set(&user_app_abort_flags[worker_idx], 0);
    k_timer_start(&user_app_soft_wd[worker_idx], K_MSEC(TIMEOUT_USER_APP_SOFT_MS), K_FOREVER);
}

void watchdog_stop_user_app(int worker_idx)
{
    if (worker_idx < 0 || worker_idx >= WORKER_POOL_SIZE) return;
    k_timer_stop(&user_app_soft_wd[worker_idx]);
    k_timer_stop(&user_app_hard_wd[worker_idx]);
    atomic_set(&user_app_abort_flags[worker_idx], 0);
}

atomic_t* watchdog_get_abort_flag(int worker_idx)
{
    if (worker_idx < 0 || worker_idx >= WORKER_POOL_SIZE) return NULL;
    return &user_app_abort_flags[worker_idx];
}

void watchdog_force_all_cooperative_abort(void)
{
    for (int i = 0; i < WORKER_POOL_SIZE; i++) {
        atomic_set(&user_app_abort_flags[i], 1);
    }
}

void watchdog_manager_init(void)
{
    for (int i = 0; i < WORKER_POOL_SIZE; i++) {
        k_timer_init(&user_app_soft_wd[i], user_app_soft_wd_expiry, NULL);
        k_timer_init(&user_app_hard_wd[i], user_app_hard_wd_expiry, NULL);
        k_timer_stop(&user_app_soft_wd[i]);
        k_timer_stop(&user_app_hard_wd[i]);
        atomic_set(&user_app_abort_flags[i], 0);
    }
}
