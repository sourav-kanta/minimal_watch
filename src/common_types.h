#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <lvgl.h>
#include "event_types.h"

#define MAX_WATCHFACES 10
#define WATCHFACE_PRIORITY 10
#define MAX_APPS 15


static const uint8_t APP_PRIORITY = 11;
static const uint32_t MAX_USER_INPUT_TIMEOUT = 5000;

/**
 * @brief Various diff timer types
 */
typedef enum {
    WF_1S_TIMER,
    WORK_WINDOW_START
} mw_timer_t;


/**
 * @brief Type to store date related info
 */
typedef struct {
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint8_t hr;
    uint8_t min;
    uint8_t sec;
    uint8_t d_week;
} date_time_t;

typedef struct {
    uint32_t last_sync_time;
    uint32_t time_sync_uptime;
    uint8_t valid;
} time_sync_t;


typedef struct {
    int16_t temperature; // Scaled by 10      
    uint8_t humidity;          
    uint8_t precip_prob;       
    uint8_t weather_code;      
    uint8_t wind_speed;        
} hourly_weather_t;

typedef struct {
    uint32_t expires_at;                 
    hourly_weather_t hourly_today[24];   
} weather_sync_t;

typedef struct {
    time_sync_t time_state;
    weather_sync_t weather_state;
} watch_state_t;

typedef struct {
    uint8_t app_id;
    uint16_t events_perms;
    const char *name;
    const lv_image_dsc_t *ico;
    void (*draw_app) (lv_obj_t *);
    void (*close_app) ();
    void (*refresh_app) ();
    void (*handle_event) (event_t*);
} application_t;

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
    const char *name;
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

#endif
