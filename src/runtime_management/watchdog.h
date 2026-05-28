#ifndef WATCHDOG_MANAGER_H
#define WATCHDOG_MANAGER_H

#include "runtime_types.h"

void watchdog_manager_init(void);
void watchdog_start_user_app(int worker_idx);
void watchdog_stop_user_app(int worker_idx);
atomic_t* watchdog_get_abort_flag(int worker_idx);
void watchdog_force_all_cooperative_abort(void);

#endif /* WATCHDOG_MANAGER_H */
