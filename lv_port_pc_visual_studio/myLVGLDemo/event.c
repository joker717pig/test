#include "event.h"
#include "lvgl.h"
#include "ui.h"
#include <stdio.h>


//void ui_event_button_cb(lv_event_t* e)
//{
//    lv_event_code_t code = lv_event_get_code(e);
//
//    ui_page_ctx_t* ctx =
//        (ui_page_ctx_t*)lv_event_get_user_data(e);
//
//    if (code == LV_EVENT_CLICKED)
//    {
//        lv_obj_t* target = lv_event_get_target(e);
//        if (target == ctx->button_a)
//        {
//            lv_label_set_text(
//                ctx->title,
//                "Button A Clicked"
//            );
//        }
//        else if (target == ctx->button_b)
//        {
//            lv_label_set_text(
//                ctx->title,
//                "Button B Clicked"
//            );
//        }
//        lv_obj_set_style_bg_color(target, lv_color_hex(0x00A86B), 0);
//    }
//}
