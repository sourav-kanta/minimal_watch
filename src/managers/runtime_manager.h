#ifndef RUNTIME_MANAGER_H
#define RUNTIME_MANAGER_H

#include "runtime_types.h"

int add_user_work(struct runtime_work_item *item);
int add_system_work(struct runtime_work_item *item);

void set_runtime_state_with_hooks(runtime_state_t target_state, 
                                  curfew_hook_t hooks[], uint8_t hook_count);
                                  
void runtime_manager_init(void);
runtime_state_t runtime_get_state(void);

uint32_t runtime_manager_get_maint_queue_depth(void);
void runtime_manager_demote_to_curfew(void);

#endif /* RUNTIME_MANAGER_H */
