#ifndef TIMERS_H
#define TIMERS_H

/**
 * @brief Various diff timer types
 */
typedef enum {
    WF_1S_TIMER
} timer_type;

void start_timers();
void stop_timers();
void stop_specific_timer(timer_type);
void start_specific_timer(timer_type);

#endif
