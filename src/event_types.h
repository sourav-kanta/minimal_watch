#ifndef EVENTS_H
#define EVENTS_H

typedef enum {
    EVENT_TICK_UPDATE,
    EVENT_WORK_TICK,
} event_id_t;

typedef struct {
    event_id_t ev;
    uint32_t payload_len;
    void* data;
} event_t;

#endif
