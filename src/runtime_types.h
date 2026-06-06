#ifndef RUNTIME_TYPES_H
#define RUNTIME_TYPES_H

#include <zephyr/kernel.h>

#define WORKER_POOL_SIZE          10  
#define WORKER_STACK_SIZE         2048  
#define BASELINE_PRIORITY         11  

#define TIMEOUT_USER_APP_SOFT_MS  30
#define TIMEOUT_USER_APP_HARD_MS  10
#define WINDOW_BACKGROUND_MAX_MS  160  
#define WINDOW_GRACE_PERIOD_MS    700   
#define WINDOW_UI_MAX_MS          300   

#define MAX_WORKER_ARG_PAYLOAD    264

#define MAX_CURFEW_HOOKS          4

typedef enum {
    WORK_TYPE_SYSTEM,
    WORK_TYPE_USER
} runtime_work_type_t;

typedef enum {
    STATE_SLEEP,
    STATE_BACKGROUND_ACTIVE,
    STATE_GRACE_PERIOD,
    STATE_UI_ACTIVE
} runtime_state_t;

struct runtime_work_item {
    runtime_work_type_t type; /* EXPLICIT FIX: Structural separation of workload categories */
    int priority;           
    void (*handler)(void *arg1, atomic_t *abort_flag);    
    void *arg1;  
    uint8_t arg_payload[MAX_WORKER_ARG_PAYLOAD];    
};

typedef void (*curfew_hook_t)(void);

#endif /* RUNTIME_TYPES_H */
