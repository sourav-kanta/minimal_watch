#include <zephyr/logging/log.h>
#include "ui/watchface/wf_picker.h"

LOG_MODULE_REGISTER(wf_picker, LOG_LEVEL_INF);

/**
 * @brief Maintain all dynamically available watchfaces
 */
static watchface_t* all_wfs[MAX_WATCHFACES];
static watchface_t* selected_wf = NULL;
/**
 * @brief Stores total subscribed watchfaces
 */
int num_wfs=-1;

/**
 * @brief Dynamically subscribe a watchface with the SYS_INIT
 * macro, makes it so that watchface can be decouupled from
 * the manager
 *
 * @param wf Watchface to be added
 */
void add_wf(watchface_t* wf) {
    if(num_wfs == (MAX_WATCHFACES - 1)) {
        __ASSERT(false, "Too many watchfaces");
        return;
    }
    wf->wf_id = ++num_wfs;
    all_wfs[num_wfs] = wf;
    LOG_INF("Adding watchfaces : %s", wf->name);
}

watchface_t* select_wf() {
    LOG_INF("No of watchfaces : %d", num_wfs+1);
    __ASSERT(num_wfs>=0, "No WF found");
    // @todo get from settings
    selected_wf = all_wfs[1];
    return selected_wf;
}


