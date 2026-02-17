#ifndef UI_MANAGER_H
#define UI_MANAGER_H


#include <lvgl.h>
#include "ui/types.h"

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
    /**
     * @brief Keep track of end of pages
     */
    _END
} ui_state;

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
} display_state;


void init_ui();
void update_ui(wf_event);
ui_state get_current_ui_state();
void deinit_ui();

#endif
