#ifndef UI_TYPES_H
#define UI_TYPES_H

#include <lvgl.h>
#include "common_types.h"

/**
 * @brief Type to describe current watch page 
 */
typedef enum UI_STATE {
    /**
     * @brief stores start of the enum for page switching
     */
    _START,
    /**
     * @brief Watchface UI
     */
    WATCHFACE,
    /**
     * @brief Application page
     */
    APP,
    NOTIFY,
    /**
     * @brief Keep track of end of pages
     */
    _END
} ui_state_t;

typedef enum DISPLAY_STATE {
    /**
     * @brief Display not initialized
     */
    DIS_INVALID,
    /**
     * @brief Display turned off
     */
    OFF,
    /**
     * @brief Display turned on
     */
    ON
} display_state_t;

typedef struct {
    date_time_t time;
    hourly_weather_t weather;
} wf_update_payload_t;

#endif
