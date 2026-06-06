#include <common_utils.h>
#include <string.h>
#include "managers/notification_manager.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(notification_mgr, LOG_LEVEL_INF);

#define MAX_NOTIFICATION 20

static notification_t all_noti[MAX_NOTIFICATION];
static unsigned int noti_len = 0;
static unsigned int write_idx = MAX_NOTIFICATION;

static void assign_handlers(notification_t* notification) {
    switch(notification->app) {
        case NAVIGATION :
            notification->action_handler = notification_init;
            notification->dismiss_handler = dismiss_notification;
            break;
        case CALL :
            notification->action_handler = notification_init;
            notification->dismiss_handler = dismiss_notification;
            break;
        default :
            notification->dismiss_handler = dismiss_notification;
    }
}

void receive_notification(notification_t* noti) {
    if(write_idx == 0) {
        for(int i = 0; i < MAX_NOTIFICATION - 1; i++) {
            memmove(&all_noti[i], &all_noti[i+1], sizeof(notification_t));
        }
        write_idx++;
        noti_len--;
        LOG_INF("Dropped last notification"); 
    }    
    write_idx--;
    memcpy(&all_noti[write_idx], noti, sizeof(notification_t));
    assign_handlers(&all_noti[write_idx]);
    noti_len++;
}

const notification_t* get_all_notifications(unsigned int* len) {
    *len = noti_len;
    return write_idx>=MAX_NOTIFICATION ? NULL : &all_noti[write_idx];
}

unsigned int get_total_notifications() {
    return noti_len;
}

const notification_t* retreive_notification(unsigned int idx) {
    if(idx + write_idx >= MAX_NOTIFICATION) 
        return NULL;
    else 
        return &all_noti[write_idx + idx];
}

void dismiss_notification(unsigned int idx) {
    if(idx>=noti_len) {
        LOG_ERR("Invalid notification idx");
        return;
    }

    for(int i=write_idx + idx; i>write_idx; i--) {
        memmove(&all_noti[i], &all_noti[i-1], sizeof(notification_t));
    }
    write_idx++;
    noti_len--;
}

void notification_init() {
}
