#ifndef WORKER_MGMT_H
#define WORKER_MGMT_H

#include "runtime_types.h"

void worker_mgmt_init(void);
void runtime_manager_interrupt_settlement(void);
void runtime_manager_evaluate_early_curfew(void);
void worker_mgmt_signal_window_start(int64_t uptime);
void worker_mgmt_stop_timers(void);

#endif /* WORKER_MGMT_H */
