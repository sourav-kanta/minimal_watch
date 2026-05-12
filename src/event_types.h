#ifndef EVENTS_H
#define EVENTS_H

typedef enum {
    TICK_UPDATE,
    BLE,
    SENSOR_DATA
} event_id_t;

typedef struct {
    event_id_t ev;
    uint32_t payload_len;
    void* data;
} event_t;

#endif
