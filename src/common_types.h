#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <lvgl.h>
#include "event_types.h"

static const uint32_t MAX_USER_INPUT_TIMEOUT = 5000;

/**
 * @brief Various diff timer types
 */
typedef enum {
    WF_1S_TIMER,
    BLE_ANCHOR_START,
    WORK_END_DEADLINE
} mw_timer_t;


/**
 * @brief Type to store date related info
 */
typedef struct {
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t hr;
    uint8_t min;
    uint8_t sec;
} date_time_t;


typedef struct {
    uint32_t last_sync_time;
    uint32_t time_sync_cpu_cycles;
} mw_state_t;

typedef struct {
    uint8_t app_id;
    uint16_t events_perms;
    const char *name;
    const lv_image_dsc_t *ico;
    void (*draw_app) (lv_obj_t *);
    void (*close_app) ();
    void (*refresh_app) ();
    void (*handle_event) ();
} application_t;

static const uint8_t APP_PRIORITY = 80;
#define MAX_APPS 15

/**
 * @brief Watchface data structure
 * which each watchface needs to register with
 */
typedef struct {
    uint8_t wf_id;
    uint16_t wf_perms;
    /**
     * @brief Name of watchface
     */
    char *name;
    /**
     * @brief function pointer to draw wf
     *
     * @param root lvgl object
     */
    void (*draw_watchface) (lv_obj_t*);
    /**
     * @brief function pointer to refresh watchface
     *
     * @param event for which update is called  
     */
    void (*update_watchface) (event_t*);
    /**
     * @brief Delete the current watchface
     *
     * @param root lvgl object
     */
    void (*del_watchface) (lv_obj_t*); 
} watchface_t;

#define MAX_WATCHFACES 10
#define WATCHFACE_PRIORITY 10

#endif
