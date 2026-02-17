#ifndef UI_TYPES_H
#define UI_TYPES_H

#include <lvgl.h>
#include "common_types.h"

/**
 * @brief Watchface data structure
 * which each watchface needs to register with
 */
typedef struct {
    /**
     * @brief Name of watchface
     */
    char *name;
    /**
     * @brief function pointer to hook event callback  
     *
     * @param event type
     */
    void (*register_events_hooks) (int8_t); 
    /**
     * @brief function pointer to draw wf
     *
     * @param root lvgl object
     */
    void (*draw_watchface) (lv_obj_t*);
    /**
     * @brief function pointer to refresh watchface
     *
     * @param root lvgl object
     * @param wf_event event for which update is called  
     */
    void (*update_watchface) (lv_obj_t*,
            wf_event);
    /**
     * @brief Delete the current watchface
     *
     * @param root lvgl object
     */
    void (*del_watchface) (lv_obj_t*); 
} watchface;

#endif
