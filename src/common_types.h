#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

typedef enum {
    UI_WF_TIMER_UPDATE,
} events;

/**
 * @brief Intermodule communication struct type, we 
 * use this to propagate events from down the stream
 * to application and watchface developers
 */
typedef struct {
    /**
     * @brief Type of event trigerred
     */
    events event_type;
    /**
     * @brief A void pointer to a struct whose type is 
     * determined by the type of event triggered\n
     * Current mappings are : 
     * <table>
     * <caption id="multi_row"> Event mappings </caption>
     * <tr><th>Event <th>Struct type</th>
     * <tr><td>Watchface 1s update <td> date_time
     * </table>
     * @todo some form of size check to prevent developer 
     * from assigning a wrong struct type
     */
    void *data;
} wf_event;

/**
 * @brief Type to store date related info
 */
typedef struct {
    int8_t day;
    int8_t month;
    int8_t year;
    int8_t hr;
    int8_t min;
    int8_t sec;
} date_time;


typedef struct {
    const char *name;
    const lv_image_dsc_t *ico;
    int8_t events_perms;
    void (*draw_app) (lv_obj_t *);
    void (*close_app) ();
    void (*refresh_app) (lv_obj_t *);
    void (*handle_event) ();
} application;

static const uint8_t APP_PRIORITY = 80;
#define MAX_APPS 15

#endif
