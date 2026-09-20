#include "PelFlo_Ass_ui_event.h"
#include "PelFlo_Ass_ui.h"
#include <string.h>
#include "Frame_enent.h"
#include "basic.h"
#include "app_keypad.h"   /* [我们] 按键 group */
#include "emg_force.h"     /* 阶段 3.3: 实时力度包络 */
#include "emg_dsp.h"       /* EMG_DSP_RAW16_LSB_UV 换算 */
#include "therapy_eval.h"  /* [我们] 盆底评估引擎（4 阶段状态机 + 指标） */



#define FILTER_WIN 7  // 滑动窗口大小（3~7 效果较好）

static int32_t filter_buf[FILTER_WIN] = { 0 };
static uint8_t filter_idx = 0;

/* [我们] 评估阶段式波形显示状态：阶段切换时清空重画，每阶段独立一条曲线 */
static eval_state_t s_plot_phase = EVAL_IDLE;   /* 当前正在画的评估阶段 */
static uint16_t     s_plot_idx   = 0;           /* 下次要画的起始索引（跟随引擎阶段进度） */

extern lv_my_ui_page_date_t* g_page_list[APP_SUM];
//extern Ui_Widget_t ui_widget;
static void hook_division_lines(lv_event_t* e);
static void Myadd_faded_area(lv_event_t* e);
static int32_t smooth_value(int32_t new_val);


/**
 * @brief 历史查看按钮位置反向补偿 作用：页面滑动时 按钮位置保持不动
 * @param e
 */
void Table_Header_scroll_cb(lv_event_t* e)
{
    lv_obj_t* page = lv_event_get_target(e);
    Scroll_Data_t* data = (Scroll_Data_t*)lv_event_get_user_data(e);

    lv_coord_t sy = lv_obj_get_scroll_y(page);
    int32_t pos = 0;
    pos = data->Origin_Y + sy;
    /*if (pos >= 420) pos = 420;*/
    lv_obj_set_pos(data->Obj, data->Origin_X, pos);
    LV_LOG_USER("Table_Header_origin_y = %d sy:%d\r\n", pos,sy);
}

/**
 * @author zqt
 * @brief 盆底评估第1页按钮焦点事件回调函数
 * @param e
 */
void AssPage1_Btn_FocEvent_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);                 // 触发的控件
    lv_event_code_t code = lv_event_get_code(e);            // 触发的事件
  
    if (code == LV_EVENT_FOCUSED) {                         //对焦事件
        if (obj == Ass_Widget.History_Btn) {
            /*给按钮设置背景颜色*/
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x68D0C3), LV_PART_MAIN);
            lv_obj_set_style_bg_color(Ass_Widget.Ass_Btn, lv_color_hex(0xffffff), LV_PART_MAIN);
            /*给按钮文字设置字体颜色*/
            lv_obj_set_style_text_color(Ass_Widget.Ass_Btn_Label, lv_color_hex(0x68D0C3), LV_PART_MAIN);
            lv_obj_set_style_text_color(Ass_Widget.History_Btn_Label, lv_color_hex(0xffffff), LV_PART_MAIN);
        }
        else  if (obj == Ass_Widget.Ass_Btn) {              /* [我们] 补上开始评估按钮对焦高亮（同事原注释掉） */
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x68D0C3), LV_PART_MAIN);
            lv_obj_set_style_bg_color(Ass_Widget.History_Btn, lv_color_hex(0xffffff), LV_PART_MAIN);
            lv_obj_set_style_text_color(Ass_Widget.Ass_Btn_Label, lv_color_hex(0xffffff), LV_PART_MAIN);
            lv_obj_set_style_text_color(Ass_Widget.History_Btn_Label, lv_color_hex(0x68D0C3), LV_PART_MAIN);
        }
    }
    else if (code == LV_EVENT_KEY) {                        /* [我们] 2D 导航 + ESC 返回 */
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_LEFT && obj == Ass_Widget.Ass_Btn) {
            lv_group_focus_obj(Ass_Widget.History_Btn);
        }
        else if (key == LV_KEY_RIGHT && obj == Ass_Widget.History_Btn) {
            lv_group_focus_obj(Ass_Widget.Ass_Btn);
        }
                else if (key == LV_KEY_ESC) {
                    LV_LOG_USER("[Ass] ESC pressed, showing confirm");
                    Ass_ShowBackConfirm();
                }
    }
    else if (code == LV_EVENT_CLICKED) {                     //点击事件
        lv_obj_remove_state(obj, LV_STATE_CHECKED);
        if (obj == Ass_Widget.History_Btn) {
            // Ass_Page_Load(AssPage5);                        //创建健康档案
            return;
        }
        else if (obj == Ass_Widget.Ass_Btn) {
        
            emg_force_reset();                              /* 清零滤波器/包络状态 */
            emg_force_start_acq(500, 1);                   // 启动采集 (500SPS, CH1)
            eval_start();                                    /* [我们] 启动评估状态机（4 阶段） */
            Ass_Page_Load(AssPage2);                        //创建开始评估页
            if (Ass_Widget.Chart_Timer != NULL) {
                lv_timer_resume(Ass_Widget.Chart_Timer);                    //开启开始评估定时器
            }
            else {
                LV_LOG_WARN("Trying to resume Chart_Timer a NULL timer!"); // 打印警告日志
            }
            
            return;
        }
      
    }
    
}
/**
 * @author zqt
 * @brief 返回菜单页按钮事件回调函数
 * @param e
 */
void BackToMenu_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);                     // 触发的控件
    lv_event_code_t code = lv_event_get_code(e);                // 触发的事件
    static lv_obj_t* lastobj1 = NULL; // 记录上一次的事件部件

    
    if (code == LV_EVENT_FOCUSED) {                         //对焦事件
        /* [我们] 防悬垂：Page1 的按钮在进入 Page4/Page5 后已删除，只有 Page1 激活时才有高亮逻辑 */
        if (lv_obj_is_valid(Ass_Widget.Ass_Btn) && lv_obj_is_valid(Ass_Widget.History_Btn_Label)) {
            if (obj == basic_widget.Record_BackBtn) {
                /*给按钮设置背景颜色*/
                lv_obj_set_style_bg_color(obj, lv_color_hex(0x68D0C3), LV_PART_MAIN);
                lv_obj_set_style_bg_color(Ass_Widget.Ass_Btn, lv_color_hex(0xffffff), LV_PART_MAIN);
                /*给按钮文字设置字体颜色*/
                lv_obj_set_style_text_color(Ass_Widget.Ass_Btn_Label, lv_color_hex(0x68D0C3), LV_PART_MAIN);
                lv_obj_set_style_text_color(Ass_Widget.History_Btn_Label, lv_color_hex(0xffffff), LV_PART_MAIN);
            }
            else  if (obj == basic_widget.Report_BackBtn) {
                /*给按钮设置背景颜色*/
                lv_obj_set_style_bg_color(obj, lv_color_hex(0x68D0C3), LV_PART_MAIN);
                lv_obj_set_style_bg_color(Ass_Widget.History_Btn, lv_color_hex(0xffffff), LV_PART_MAIN);
                /*给按钮文字设置字体颜色*/
                lv_obj_set_style_text_color(Ass_Widget.Ass_Btn_Label, lv_color_hex(0xffffff), LV_PART_MAIN);
                lv_obj_set_style_text_color(Ass_Widget.History_Btn_Label, lv_color_hex(0x68D0C3), LV_PART_MAIN);
            }
        }
    }
    else if (code == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {   /* [我们] ESC 返回 */
        Ass_ShowBackConfirm();
    }
    else if (code == LV_EVENT_CLICKED) {                    //点击事件

        if (obj == basic_widget.Record_BackBtn)             //返回菜单按钮
        {
            /*  lv_obj_remove_state(obj, LV_STATE_CHECKED);*/
            Ass_ShowBackConfirm();
          
        }
        else if (obj == basic_widget.Report_BackBtn)         //返回菜单按钮
        {
            lv_obj_remove_state(obj, LV_STATE_CHECKED);
            Ass_ShowBackConfirm();
        } 
    } 
}

/* [我们] 无按钮页面的 ESC 处理（评估中/生成报告页） */
void Ass_Page_Esc_cb(lv_event_t* e)
{
    LV_LOG_USER("[Ass] Ass_Page_Esc_cb entered");
    if (lv_event_get_code(e) == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        LV_LOG_USER("[Ass] Page2/3 ESC pressed, showing confirm");
        Ass_ShowBackConfirm();
    }
}

/**
 * @brief 历史查看按钮位置反向补偿 作用：页面滑动时 按钮位置保持不动
 * @param e 
 */
void HisBtn_scroll_cb(lv_event_t* e)
{
    lv_obj_t* page = lv_event_get_target(e);
    lv_obj_t* btn = lv_event_get_user_data(e);

    lv_coord_t sy = lv_obj_get_scroll_y(page);
    uint16_t pos = 0;
    pos = HisBtn_origin_y + sy;
    if (pos >= 420) pos = 420;
    lv_obj_set_pos(btn, 195, pos);
  /*  LV_LOG_USER("HisBtn_origin_y = %d\r\n", pos);*/
}

/**
 * @brief 开始评估按钮位置反向补偿 作用：页面滑动时 按钮位置保持不动
 * @param e
 */
void AssBtn_scroll_cb(lv_event_t* e)
{
    lv_obj_t* page = lv_event_get_target(e);
    lv_obj_t* btn = lv_event_get_user_data(e);

    lv_coord_t sy = lv_obj_get_scroll_y(page);
    uint16_t pos = 0;
    pos = AssBtn_origin_y + sy;
    if (pos >= 420) pos = 420;
    lv_obj_set_pos(btn, 324, pos);
    /*LV_LOG_USER("AssBtn_origin_y = %d\r\n", AssBtn_origin_y + sy);*/
}
/**
 * @brief 模糊容器位置反向补偿 作用：页面滑动时 位置保持不动
 * @param e
 */
void Obj_scroll_cb(lv_event_t* e)
{
    lv_obj_t* page = lv_event_get_target(e);
    lv_obj_t* btn = lv_event_get_user_data(e);

    lv_coord_t sy = lv_obj_get_scroll_y(page);
    uint16_t pos = 0;
    pos = Obj_origin_y + sy;
    if (pos >= 445) pos = 445;
    lv_obj_set_pos(btn, -8, pos);
    //LV_LOG_USER("Obj_origin_y = %d\r\n", pos);
}
/**
 * @brief 记录页菜单项某一项生成数据回调函数
 * @param  e:
 */
void Record_Menu_event_cb(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_target(e);
    lv_menu_page_t* page = (lv_menu_page_t*)target;

    uint8_t index = (uint8_t)(uintptr_t)lv_event_get_user_data(e);

    if (lv_event_get_code(e) != LV_EVENT_STYLE_CHANGED) return;
    lv_obj_t* obj = lv_menu_section_create(target);
    lv_obj_set_style_bg_color(obj, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_set_size(obj, 480, 300);
    lv_obj_set_flex_grow(obj, 0);                  /* 1. 关 flex grow，让 title 不被 flex 拉伸 */
    lv_obj_set_style_margin_top(obj, -25, 0);
    
}
/**
 * @brief 报告生成 定时器跳转函数
 * @param t
 */
void report_timer_cb(lv_timer_t* t)
{
    lv_obj_t* page_cont = lv_timer_get_user_data(t);
    static uint8_t count = 0;
   /* lv_my_child_page_date_t* page_data = lv_obj_get_user_data(page_cont);*/
     LV_LOG_USER("report_timer_cb:%d",count);
     count++;
    if (count >= 30) {
        count = 0;
        Ass_Page_Load(AssPage4);                       //创建检测报告页
        lv_timer_pause(Ass_Widget.Report_Timer);                //暂停定时器
    }
}
/**
 * @author zqt 
 * @brief 折线图定时器回调函数（[我们] 改为评估状态机驱动：阶段/倒计时/结束均由 eval 引擎决定）
 * @param t 
 */
void Aemg_add_data(lv_timer_t* t)
{
    lv_obj_t* chart = lv_timer_get_user_data(t);
    lv_chart_series_t* ser = lv_chart_get_series_next(chart, NULL);
    if (ser == NULL) return;

    /* 实时力度包络 (raw16 LSB → µV) */
    int32_t env_uv = (int32_t)(emg_force_get_env() * EMG_DSP_RAW16_LSB_UV);

    lv_label_set_text_fmt(Ass_Widget.Instan_Label, "%d", (int)env_uv);   // 瞬时肌电位

    /* 当前评估阶段名 */
    if (Ass_Widget.Phase_Label) {
        lv_label_set_text(Ass_Widget.Phase_Label, eval_phase_name());
    }

    /* [我们] 当前/下节动作名 */
    if (Ass_Widget.Action_Label) {
        lv_label_set_text_fmt(Ass_Widget.Action_Label, "%s", eval_action_name());
    }
    if (Ass_Widget.Next_Action_Label) {
        lv_label_set_text_fmt(Ass_Widget.Next_Action_Label, "%s", eval_next_action_name());
    }

    /* 全程总剩余时间（分:秒，从约 4:32 起倒计时） */
    if (Ass_Widget.RemTime_Label) {
        uint16_t rem = eval_total_remain_s();
        lv_label_set_text_fmt(Ass_Widget.RemTime_Label, "%d分%02d秒", rem / 60, rem % 60);
    }

    /* ⭐ 阶段切换检测：每个评估阶段（前静息/快速收缩/持续收缩/后静息）清空重画一条独立曲线 */
    eval_state_t st = eval_get_state();
    if (st != s_plot_phase) {
        s_plot_phase = st;
        if (st == EVAL_REST1 || st == EVAL_FAST || st == EVAL_SUST || st == EVAL_REST2) {
            Ass_Guide_Update((uint8_t)(st - 1));   /* [我们] 阶段切换：重画该阶段引导波形 */
            uint16_t total_s = eval_phase_total_s();
            uint16_t n = (uint16_t)((uint32_t)total_s * 20u);   /* 50ms 一点 = 20 点/秒 */
            if (n < 2) n = 2;
            lv_chart_set_point_count(chart, n);                 /* 点数=本阶段时长 → 曲线画满整屏 */
            lv_chart_set_all_value(chart, ser, LV_CHART_POINT_NONE);  /* 清空旧数据 */
            lv_chart_set_x_start_point(chart, ser, 0);          /* 从最左开始画 */
            s_plot_idx = 0;
            /* 清空滑动均值窗口，防跨阶段混入旧值 */
            memset(filter_buf, 0, sizeof(filter_buf));
            filter_idx = 0;
        }
    }

    /* 阶段内：跟随引擎阶段进度定位画点（idx=阶段内已过 tick），补齐中间点，
     *       确保阶段结束正好画满到最右（不依赖 UI 自己的计数器，避免与引擎 tick 漂移少画尾点） */
    int32_t smoothed = smooth_value(env_uv);
    uint16_t idx = eval_phase_elapsed_ticks();
    uint16_t max_idx = lv_chart_get_point_count(chart);
    if (max_idx == 0) max_idx = 1;
    if (idx >= max_idx) idx = (uint16_t)(max_idx - 1);
    for (uint16_t i = s_plot_idx; i <= idx; i++) {
        lv_chart_set_value_by_id(chart, ser, i, smoothed);
    }
    s_plot_idx = (uint16_t)(idx + 1);

    /* 评估完成（4 阶段全部结束）: 停止采集 -> 生成报告 */
    if (st == EVAL_DONE) {
        emg_force_stop_acq();
        /* [我们] 评估完成：Chart_Timer 无用（chart 即将被 Page_Clean 删除），直接删除防悬垂/泄漏 */
        if (Ass_Widget.Chart_Timer) { lv_timer_del(Ass_Widget.Chart_Timer); Ass_Widget.Chart_Timer = NULL; }
        Ass_Page_Load(AssPage3);                            // 创建等待生成报告页
        lv_timer_resume(Ass_Widget.Report_Timer);            // 开启生成报告定时器
    }
}


/**
 * @brief 数据表格回调函数
 * @param  e:
 */
void Draw_EcgTable_event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_user_data(e);
    lv_draw_task_t* draw_task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t* base_dsc = lv_draw_task_get_draw_dsc(draw_task);
    /*If the cells are drawn...*/
    if (base_dsc->part == LV_PART_ITEMS) {
        uint32_t row = base_dsc->id1;
        uint32_t col = base_dsc->id2;

        /*使第一个单元格居中的文本对齐*/
        if (row == 0 && obj != NULL) {
            lv_draw_label_dsc_t* label_draw_dsc = lv_draw_task_get_label_dsc(draw_task);
            if (label_draw_dsc) {
                label_draw_dsc->align = LV_TEXT_ALIGN_CENTER;
            }
           /* lv_draw_fill_dsc_t* fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
            if (fill_draw_dsc) {
                fill_draw_dsc->color = lv_color_mix(lv_palette_main(LV_PALETTE_BLUE), fill_draw_dsc->color, LV_OPA_20);
                fill_draw_dsc->opa = LV_OPA_COVER;
            }*/
        }
        /*使每隔一行显示灰色*/
        if ((row != 0 && row % 2) != 0) {
            lv_draw_fill_dsc_t* fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
            if (fill_draw_dsc) {
                fill_draw_dsc->color = lv_color_mix(lv_color_hex(0xF2F4FA), fill_draw_dsc->color, LV_OPA_100);
                fill_draw_dsc->opa = LV_OPA_COVER;
            }
        }

        lv_draw_label_dsc_t* label_draw_dsc = lv_draw_task_get_label_dsc(draw_task);
        if (label_draw_dsc) {
            label_draw_dsc->align = LV_TEXT_ALIGN_CENTER;//LV_TEXT_ALIGN_RIGHT;
        }
    }
}

/**
 * @brief 曲线图绘图事件
 * @param e 
 */
void Mydraw_event_cb(lv_event_t* e)
{
    lv_draw_task_t* draw_task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t* base_dsc = lv_draw_task_get_draw_dsc(draw_task);
    lv_obj_t* obj = lv_event_get_user_data(e);

    /*是否为画线任务*/
    if (base_dsc->part == LV_PART_ITEMS && lv_draw_task_get_type(draw_task) == LV_DRAW_TASK_TYPE_LINE) {
        Myadd_faded_area(e);

    }
    /*Hook the division lines too*/
    if (base_dsc->part == LV_PART_MAIN && lv_draw_task_get_type(draw_task) == LV_DRAW_TASK_TYPE_LINE) {
        hook_division_lines(e);
    }
}

static void Myadd_faded_area(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    lv_draw_task_t* draw_task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t* base_dsc = lv_draw_task_get_draw_dsc(draw_task);

    const lv_chart_series_t* ser = lv_chart_get_series_next(obj, NULL);
    if (ser == ser_red) return;
    lv_color_t ser_color = lv_chart_get_series_color(obj, ser);
   
    lv_draw_line_dsc_t* line_dsc = lv_draw_task_get_draw_dsc(draw_task);

    /* 1. 获取关键点坐标 */
    float p1x = line_dsc->p1.x;
    float p1y = line_dsc->p1.y;
    float p2x = line_dsc->p2.x - 0.5f ;
    float p2y = line_dsc->p2.y;

    /* Y=0 (X轴) 在屏幕上的像素位置 */
    float zero_y = coords.y2;

    /* 2. 准备绘图描述符 */
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.radius = 0; /* 矩形无圆角 */
    rect_dsc.border_width = 0;
    rect_dsc.bg_grad.dir = LV_GRAD_DIR_VER;

    lv_draw_triangle_dsc_t tri_dsc;
    lv_draw_triangle_dsc_init(&tri_dsc);
    tri_dsc.grad.dir = LV_GRAD_DIR_VER;   /* [我们] LVGL9.5: bg_grad -> grad */

    /* 3. 计算渐变参数 */
    int32_t full_h = lv_obj_get_height(obj);

    /* 矩形部分：顶部透明度取决于线段的最低点，底部完全透明 */
    float rect_top_ratio = (LV_MIN(p1y, p2y) - coords.y1) / (float)full_h;
    uint8_t rect_top_opa = (uint8_t)((1.0f - rect_top_ratio) * 100);
    uint8_t rect_btm_opa = 0; /* X轴处完全透明 */

    /* 三角形部分：顶部透明度取决于线段的最高点，底部与矩形顶部对齐 */
    float tri_top_ratio = (LV_MAX(p1y, p2y) - coords.y1) / (float)full_h;
    uint8_t tri_top_opa = (uint8_t)((1.0f - tri_top_ratio) * 100);

    /* 4. 绘制矩形（覆盖折线正下方的垂直区域） */
    /* 矩形的左右边界就是线段端点的X坐标，上下边界是从X轴到线段最低点 */
    lv_area_t rect_area;
    rect_area.x1 = (int32_t)((p1x < p2x) ? p1x : p2x);
    rect_area.x2 = (int32_t)((p1x > p2x) ? p1x : p2x);
    rect_area.y1 = (int32_t)LV_MAX(p1y, p2y); /* 上边界：线段较低的一端 */
    rect_area.y2 = (int32_t)zero_y;           /* 下边界：X轴 */

    /* 只有当线段不是水平线时才画矩形，否则矩形高度为0会导致无效果 */
    if (rect_area.y1 < rect_area.y2) {
        rect_dsc.bg_grad.stops[0].color = ser_color;
        rect_dsc.bg_grad.stops[0].opa = rect_top_opa;
        rect_dsc.bg_grad.stops[0].frac = 0;
        rect_dsc.bg_grad.stops[1].color = ser_color;
        rect_dsc.bg_grad.stops[1].opa = rect_btm_opa;
        rect_dsc.bg_grad.stops[1].frac = 255;

        lv_draw_rect(base_dsc->layer, &rect_dsc, &rect_area);
    }

    /* 5. 绘制三角形（填补矩形上方与折线之间的空隙） */
    /* 这是一个标准的直角三角形，直角在下方 */
    tri_dsc.p[0].x = p1x; tri_dsc.p[0].y = p1y;
    tri_dsc.p[1].x = p2x + 0.5f; tri_dsc.p[1].y = p2y;
   // 找到折线两个点中较低的那个（Y值较大的）
    float lower_y = LV_MAX(p1y, p2y);
    // 这个数字你可以调（2, 3, 5...），直到你看到填充出现在折线下方
    lower_y += 1.0f;

    if (p2y > p1y) {
        // 下降：直角顶点在右下角（X取p2x，Y取我们修正过的lower_y）
        tri_dsc.p[2].x = p1x;
        tri_dsc.p[2].y = lower_y;
    }
    else {
        // 上升：直角顶点在左下角（X取p1x，Y取我们修正过的lower_y）
        tri_dsc.p[2].x = p2x + 0.5f;
        tri_dsc.p[2].y = lower_y;
    }
    tri_dsc.grad.stops[0].color = ser_color;
    tri_dsc.grad.stops[0].opa = tri_top_opa;
    tri_dsc.grad.stops[0].frac = 0;
    tri_dsc.grad.stops[1].color = ser_color;
    /* 三角形的底边透明度应该与矩形的顶边透明度一致，才能无缝衔接 */
    tri_dsc.grad.stops[1].opa = (p1y < p2y) ? rect_top_opa : rect_top_opa;
    tri_dsc.grad.stops[1].frac = 255;

    /* 只有当线段不是水平线时才画三角形 */
    if (p1y != p2y) {
        lv_draw_triangle(base_dsc->layer, &tri_dsc);
    }
    
}

static void hook_division_lines(lv_event_t* e)
{
    lv_draw_task_t* draw_task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t* base_dsc = lv_draw_task_get_draw_dsc(draw_task);
    lv_draw_line_dsc_t* line_dsc = lv_draw_task_get_draw_dsc(draw_task);


    line_dsc->width = 1;
    line_dsc->dash_gap = 2;
    line_dsc->dash_width = 2;

    line_dsc->color = lv_palette_lighten(LV_PALETTE_GREY, 1);
}
 
        

static int32_t smooth_value(int32_t new_val)
{
    filter_buf[filter_idx] = new_val;
    filter_idx = (filter_idx + 1) % FILTER_WIN;

    int32_t sum = 0;
    for (uint8_t i = 0; i < FILTER_WIN; i++) {
        sum += filter_buf[i];
    }
    return sum / FILTER_WIN;
}
