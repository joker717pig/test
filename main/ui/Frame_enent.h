#ifndef __FRAME_ENENT_H__
#define __FRAME_ENENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
    void update_time_cb(lv_timer_t* timer);
    void update_batt_cb(lv_timer_t* timer);

#ifdef __cplusplus
}
#endif

#endif
