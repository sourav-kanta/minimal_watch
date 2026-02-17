#ifndef WF_PICKER_H
#define WF_PICKER_H

#include <lvgl.h>
#include "ui/types.h"

#define WATCHFACE_PRIORITY 10

void add_wf(watchface*);
watchface* select_wf();

#endif
