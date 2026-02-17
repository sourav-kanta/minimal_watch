#include <zephyr/logging/log.h>
#include "ui/watchface/wf_picker.h"

LOG_MODULE_REGISTER(wf_picker, LOG_LEVEL_INF);

#define MAX_WATCHFACES 10

/**
 * @brief Maintain all dynamically available watchfaces
 */
static watchface* all_wfs[MAX_WATCHFACES];
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
void add_wf(watchface* wf) {
    if(num_wfs == (MAX_WATCHFACES - 1)) {
        __ASSERT(false, "Too many watchfaces");
        return;
    }
    all_wfs[++num_wfs] = wf;
    LOG_INF("Adding watchfaces : %s", wf->name);
}

watchface* select_wf() {
    LOG_INF("No of watchfaces : %d", num_wfs+1);
    __ASSERT(num_wfs>=0, "No WF found");
    return all_wfs[0];
}
