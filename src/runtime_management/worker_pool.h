#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include "runtime_types.h"

void worker_pool_init(void);
void worker_pool_resume_all(void);
void worker_pool_suspend_all(void);
k_tid_t worker_pool_get_tid(int worker_idx);
void worker_pool_recover_stalled_runner(int worker_idx);

int worker_pool_enqueue_user_item(struct runtime_work_item *item);
int worker_pool_get_pending_count(void);
int worker_pool_get_active_count(void);
bool worker_pool_has_work(void);

void worker_pool_purge_queue(void);

#endif /* WORKER_POOL_H */
