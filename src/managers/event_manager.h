/**
 * @file event_manager.h
 * @brief APIs for system events
 * @author Sourav Kanta
 * @version v1
 * @date 2026-04-08
 */

#ifndef EVENT_MANGER_H
#define EVENT_MANGER_H

#include "ble_types.h" 
#include "event_types.h"

bool request_ble_action(ble_req_t*);
void handle_event(event_t*);
void init_events();

#endif
