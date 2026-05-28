#include "runtime_management/worker_pool.h"
#include "runtime_management/worker_mgmt.h"
#include "runtime_management/watchdog.h"
#include "managers/runtime_manager.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(worker_pool, LOG_LEVEL_INF);

K_THREAD_STACK_ARRAY_DEFINE(worker_stacks, WORKER_POOL_SIZE, WORKER_STACK_SIZE);
static struct k_thread worker_threads[WORKER_POOL_SIZE];
static k_tid_t worker_tids[WORKER_POOL_SIZE]; 

K_MSGQ_DEFINE(runtime_msgq, sizeof(struct runtime_work_item), 16, 4);
static atomic_t active_worker_count = ATOMIC_INIT(0);

static void pool_worker_entry(void *p1, void *p2, void *p3);

int worker_pool_enqueue_user_item(struct runtime_work_item *item)
{
    item->type = WORK_TYPE_USER;
    return k_msgq_put(&runtime_msgq, item, K_NO_WAIT);
}

int worker_pool_get_pending_count(void)
{
    return k_msgq_num_used_get(&runtime_msgq);
}

int worker_pool_get_active_count(void)
{
    return (int)atomic_get(&active_worker_count);
}

bool worker_pool_has_work(void)
{
    return (atomic_get(&active_worker_count) > 0 || k_msgq_num_used_get(&runtime_msgq) > 0);
}

void worker_pool_purge_queue(void)
{
    k_msgq_purge(&runtime_msgq);
}

k_tid_t worker_pool_get_tid(int worker_idx)
{
    if (worker_idx < 0 || worker_idx >= WORKER_POOL_SIZE) return NULL;
    return worker_tids[worker_idx];
}

void worker_pool_resume_all(void)
{
    for (int i = 0; i < WORKER_POOL_SIZE; i++) {
        if (worker_tids[i] != NULL) {
            k_thread_resume(worker_tids[i]);
        }
    }
}

void worker_pool_suspend_all(void)
{
    for (int i = 0; i < WORKER_POOL_SIZE; i++) {
        if (worker_tids[i] != NULL) {
            k_thread_suspend(worker_tids[i]);
        }
    }
}

void worker_pool_recover_stalled_runner(int worker_idx)
{
    if (worker_idx < 0 || worker_idx >= WORKER_POOL_SIZE) return;

    LOG_WRN("[Pool] Re-spawning dead worker runner at index slot [%d]", worker_idx);
    watchdog_stop_user_app(worker_idx);

    worker_tids[worker_idx] = k_thread_create(&worker_threads[worker_idx],
                                worker_stacks[worker_idx],
                                WORKER_STACK_SIZE,
                                pool_worker_entry,
                                INT_TO_POINTER(worker_idx), NULL, NULL,
                                BASELINE_PRIORITY, 0, K_NO_WAIT);
}

static void pool_worker_entry(void *p1, void *p2, void *p3)
{
    int worker_idx = POINTER_TO_INT(p1);
    struct runtime_work_item item;

    while (1) {
        int msg_status = k_msgq_get(&runtime_msgq, &item, K_FOREVER);

        if (msg_status == 0) {
            if (runtime_get_state() == STATE_SLEEP) {
                continue;
            }

            runtime_manager_interrupt_settlement();
            atomic_inc(&active_worker_count);
            k_thread_priority_set(k_current_get(), item.priority);
            
            bool is_user_task = (item.type == WORK_TYPE_USER);
            if (is_user_task) {
                watchdog_start_user_app(worker_idx);
            }
            item.handler(item.arg1, watchdog_get_abort_flag(worker_idx));

            if (is_user_task) {
                watchdog_stop_user_app(worker_idx);
            }

            k_thread_priority_set(k_current_get(), BASELINE_PRIORITY);
            atomic_dec(&active_worker_count);
            
            if (worker_pool_get_active_count() == 0) {
                runtime_manager_evaluate_early_curfew();
            }
        }
    }
}

void worker_pool_init(void)
{
    atomic_set(&active_worker_count, 0);
    for (int i = 0; i < WORKER_POOL_SIZE; i++) {
        worker_tids[i] = k_thread_create(&worker_threads[i],
                                        worker_stacks[i],
                                        WORKER_STACK_SIZE,
                                        pool_worker_entry,
                                        INT_TO_POINTER(i), NULL, NULL,
                                        BASELINE_PRIORITY, 0, K_NO_WAIT);
        k_thread_name_set(worker_tids[i], "pool_worker");
    }
    
    /* Start suspended until an active operating state turns them on */
    worker_pool_suspend_all();
}
