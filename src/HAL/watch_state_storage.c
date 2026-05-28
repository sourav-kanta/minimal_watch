#include "common_types.h"
#include <zephyr/kernel.h>

#ifdef CONFIG_SOC_ESP32S3

RTC_DATA_ATTR static watch_state_t watch_state;

#endif

#ifdef CONFIG_ARCH_POSIX
    
static watch_state_t watch_state = {0};
    
#endif 


void assign_watch_state_addresses(watch_state_t** state) {
    *state = &watch_state;
}
