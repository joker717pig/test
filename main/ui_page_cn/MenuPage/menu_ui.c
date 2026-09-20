#include "menu_ui.h"
#include "menu_ui_event_cb.h"
#include "PelFlo_Ass_ui.h"   /* B2 盆底评估页（替换 stub） */
#include "PelFlo_Set_ui.h"   /* B3 设置页（替换 stub） */
#include "PosReh_Page_ui.h"  /* B4 产后修复页（替换 stub） */
#include "PelFlo_The_ui.h"   /* B5-C 盆底治疗页四宫格（替换 stub） */
#include "app_keypad.h"      /* [我们] keypad group 注册 */
#include "esp_log.h"         /* [我们] stub 日志 */
#include "app_therapy.h"        /* [我们] 阶段C：切页安全网（停治疗引擎） */
#include "emg_force.h"     /* 阶段 3.3: 实时力度包络 */
#include "emg_dsp.h"       /* EMG_DSP_RAW16_LSB_UV 换算 */
#include "esp_log.h"         /* [我们] stub 日志 */
#include <math.h>   // sinf()
#include <stdio.h>
static const char *TAG = "menu_ui";

static void Menu_widget(Menu_Widget_t* widget, lv_obj_t* page_cont);
/* [我们] B5-B 对齐 PC menu_ui.c：疗程网格单格控件前向声明（Select_Widget2 在定义前调用） */
static void Stage_Widget2(lv_obj_t* page_cont,char *s, uint8_t t, Stage_Data_t* data);
/* [我们] B5-B 对齐 PC menu_ui.c：疗程网格计数器（Select_Widget2 内部用，仅本文件） */

static uint8_t g_Treat_Point_cnt = 0;
uint8_t chart_repeat = 0;    /*波形图循环次数 */
uint8_t cur_repeat_cnt = 0;  //当前重复次数
lv_obj_t *g_guide_cont = NULL;     /* 引导波形容器（波形图底层） */
lv_obj_t *g_guide_cont_line = NULL; /* 引导波形容器（波形图底层） */
uint8_t   g_guide_phase = 0;       /* 当前阶段序号（0~3），绘制回调据此画三角峰 */
uint8_t   last_guide_phase = 0;    /* 上一个阶段序号（0~3），绘制回调据此画三角峰 */
#define THERAPY_GUIDE_HIGH_Y   22             /* 用力峰顶 y（容器内，对齐刻度 75） */
#define THERAPY_GUIDE_LOW_Y    90             /* 静息基线 y（容器内，对齐刻度 25） */
#define THERAPY_GUIDE_BAR_H    3              /* 基线横线高度（px） */
#define THERAPY_GUIDE_COLOR    0xFFA500       /* 橙色 */

void Chart_Init_Point(lv_obj_t* chart)
{
    uint8_t repeat = 5;     //跟引导波形一致
    uint8_t step_time = 0;
    uint8_t step_cnt = 0;
    uint32_t n = 0;
    lv_chart_series_t* ser = lv_chart_get_series_next(chart, NULL);
    /* 阶段总 tick（50ms 一点 = 20 点/秒） */
    uint32_t total_time  = 0;

    total_time = therapy_current_stage_time();  //获取当前阶段总时间
    total_time /= 1000;
    cur_repeat_cnt = 0; 

    step_cnt = therapy_current_step_cnt();      //获取当前阶段步骤数
    const rx_step_t *stage = cur_stage();       //获取当前步骤
    for(uint8_t i = 0; i < step_cnt; i++) {     //计算当前一个步骤需要多长时间
        if(stage[i].type == RX_STEP_TRAIN || stage[i].type == RX_STEP_TRAIN_INSTR) {
            step_time += stage[i].dur_s;
            
        }
    }
    n = repeat * step_time * 20u;               //设置图表的点数为5个步骤需要的点数
    chart_repeat = total_time * 20u / n;        //一个阶段循环5个引导图 总需要循环次数
    lv_chart_set_point_count(chart, n);                         /* 点数=本阶段时长 → 曲线画满整屏 */
    lv_chart_set_all_value(chart, ser, LV_CHART_POINT_NONE);    /* 清空旧数据 */
    lv_chart_set_x_start_point(chart, ser, 0);                  /* 从最左开始画 */
    ESP_LOGI(TAG, "获取当前阶段总时间:%d,获取当前阶段步骤数:%d,单步骤：%d,重复次数：%d,chart point=%d\r\n",
                                                    total_time,step_cnt, step_time,chart_repeat,n);
}


/**
 * @brief 引导波形绘制回调：画静息基线 + 每个"用力/收缩"步骤一个正弦半波
 *
 * 规则（与 Chart_Init_Point 的图表 x 轴对齐）：
 *   - 只取处方疗程中的"主动训练"步骤（RX_STEP_TRAIN / RX_STEP_TRAIN_INSTR）
 *   - 跳过条件刺激（RX_STEP_COND）/ 电刺激 / 等待等非训练步骤
 *   - 放松步骤（RX_TRAIN_RELAX）：只推进 x 轴，不画波（基线已画）
 *   - 用力/收缩步骤（train != RELAX）：画一个正弦半波
 *   - 整个主动训练模式重复 5 次（= 5 个引导图，与图表 repeat 一致）
 *   - x 轴比例只按主动训练步骤时长累加（与图表点数 n = repeat*step_time*20 一致）
 */
static void therapy_guide_draw_line_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_layer_t *layer = lv_event_get_layer(e);

    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    int32_t cx = coords.x1;
    int32_t cy = coords.y1;

    /* 静息基线：全宽横线（25% 刻度处） */
    lv_draw_rect_dsc_t base_dsc;
    lv_draw_rect_dsc_init(&base_dsc);
    base_dsc.base.layer = layer;
    base_dsc.radius = 0;
    base_dsc.border_width = 0;
    base_dsc.bg_color = lv_color_hex(FONT_GREEN_COLOR);
    base_dsc.bg_opa = LV_OPA_COVER;
    lv_area_t base_area;
    base_area.x1 = cx;
    base_area.x2 = cx + 369;
    base_area.y1 = cy + THERAPY_GUIDE_LOW_Y;
    base_area.y2 = cy + THERAPY_GUIDE_LOW_Y + THERAPY_GUIDE_BAR_H - 1;
    lv_draw_rect(layer, &base_dsc, &base_area);

    const rx_step_t *stage = cur_stage();
    if (!stage) return;
    uint8_t step_cnt = therapy_current_step_cnt();

    const uint8_t repeat = 5;   /* 画 5 个引导图（与 Chart_Init_Point 一致） */

    /* 主动训练（TRAIN/TRAIN_INSTR）步骤总时长 → x 轴总 tick（50ms 一点 = 20 点/秒） */
    uint32_t train_time = 0;
    for (uint8_t i = 0; i < step_cnt; i++) {
        if (stage[i].type == RX_STEP_TRAIN || stage[i].type == RX_STEP_TRAIN_INSTR) {
            train_time += stage[i].dur_s;
        }
    }
    uint32_t total_ticks = (uint32_t)repeat * train_time * 20u;

    /* 防止除零（本阶段无主动训练步骤） */
    if (total_ticks == 0) return;

    /* 线段绘制描述符（复用） */
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.base.layer = layer;
    line_dsc.color = lv_color_hex(FONT_GREEN_COLOR);
    line_dsc.opa = LV_OPA_COVER;
    line_dsc.width = 1;               /* 线宽 */
    line_dsc.round_start = 0;
    line_dsc.round_end = 0;

    /* 振幅和基线 */
    int32_t base_y = cy + THERAPY_GUIDE_LOW_Y;   /* 基线 y */
    int32_t amp = THERAPY_GUIDE_HIGH_Y - THERAPY_GUIDE_LOW_Y; /* 峰顶到基线的距离（正数） */
    if (amp < 0) amp = -amp;                     /* 防止 y 轴方向定义相反 */

    /* tick_x 跨重复持续累加：5 个引导图依次排列占满整屏（total_ticks = repeat*train_time*20） */
    uint32_t tick_x = 0;
    for (uint8_t r = 0; r < repeat; r++) {
        for (uint8_t i = 0; i < step_cnt; i++) {
            /* 只处理主动训练步骤；跳过条件刺激/电刺激/等待 */
            if (stage[i].type != RX_STEP_TRAIN && stage[i].type != RX_STEP_TRAIN_INSTR) {
                continue;
            }

            uint32_t step_ticks = (uint32_t)stage[i].dur_s * 20u;

            /* 放松步骤：只推进 x 轴，不画波（基线已画） */
            if (stage[i].train == RX_TRAIN_RELAX) {
                tick_x += step_ticks;
                continue;
            }

            /* 用力/收缩步骤：画一个正弦半波 */
            int32_t x0 = cx + (int32_t)((uint64_t)tick_x * 370u / total_ticks);
            int32_t x1 = cx + (int32_t)((uint64_t)(tick_x + step_ticks) * 370u / total_ticks);
            int32_t width = x1 - x0;
            if (width <= 0) { tick_x += step_ticks; continue; }  /* 避免宽度为 0 */

            int32_t segments = width;          /* 以宽度为单位逐点画 */
            int32_t x_prev = x0;
            int32_t y_prev = base_y;

            for (int32_t s = 0; s <= segments; s++) {
                /* 水平坐标：按比例插值，保证终点一定落在 x1 */
                int32_t x_cur = x0 + (int32_t)((int64_t)width * s / segments);
                /* 相位 0 → π */
                float phase = 3.1415926f * s / segments;
                /* 正弦波高度 */
                int32_t y_cur = base_y - (int32_t)(amp * sinf(phase));

                line_dsc.p1.x = x_prev; line_dsc.p1.y = y_prev;
                line_dsc.p2.x = x_cur;  line_dsc.p2.y = y_cur;
                lv_draw_line(layer, &line_dsc);

                x_prev = x_cur;
                y_prev = y_cur;
            }
            tick_x += step_ticks;
        }
    }
}

/**
 * @brief 引导波形绘制回调：画静息基线 + 每个"用力"步骤一个三角峰
 */
static void therapy_guide_draw_tri_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_layer_t *layer = lv_event_get_layer(e);

    /* 阶段总 tick（50ms 一点 = 20 点/秒） */
    uint32_t total_ticks = 0;

    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    int32_t cx = coords.x1;
    int32_t cy = coords.y1;

    /* 静息基线：全宽横线（25% 刻度处） */
    lv_draw_rect_dsc_t base_dsc;
    lv_draw_rect_dsc_init(&base_dsc);
    base_dsc.base.layer = layer;
    base_dsc.radius = 0;
    base_dsc.border_width = 0;
    base_dsc.bg_color = lv_color_hex(FONT_GREEN_COLOR);
    base_dsc.bg_opa = LV_OPA_COVER;
    lv_area_t base_area;
    base_area.x1 = cx;
    base_area.x2 = cx + 369;
    base_area.y1 = cy + THERAPY_GUIDE_LOW_Y;
    base_area.y2 = cy + THERAPY_GUIDE_LOW_Y + THERAPY_GUIDE_BAR_H - 1;
    lv_draw_rect(layer, &base_dsc, &base_area);

    uint8_t repeat = 5;
    uint8_t step_time = 0;
    uint8_t step_cnt = 0;
    uint8_t i = 0;
    step_cnt = therapy_current_step_cnt();
    const rx_step_t *stage = cur_stage();
    for(i = 0; i < step_cnt; i++) {
        if(stage[i].type == RX_STEP_TRAIN || stage[i].type == RX_STEP_TRAIN_INSTR) {
                step_time += stage[i].dur_s;
            // ESP_LOGI(TAG, "step_time=%d 秒 总步骤：%d,当前步骤：%d\r\n", step_time,step_cnt,i);
        }
        
    }
    step_cnt = i;
    total_ticks = repeat * step_time * 20u;
    /* 每个 TRAIN 步骤画一个三角峰：基线 → 峰顶(75%) → 基线 */
    uint32_t tick_x = 0;
    for (uint16_t r = 0; r <repeat; r++) {
        for (uint16_t i = 0; i < step_cnt; i++) {
            if(stage[i].type == RX_STEP_COND) break;        //若为条件刺激则跳过
            uint32_t step_ticks = (uint32_t)stage[i].dur_s * 20u;
            if (stage[i].train != RX_TRAIN_RELAX) {
                int32_t x0 = cx + (int32_t)((uint64_t)tick_x * 370u / total_ticks);
                int32_t x1 = cx + (int32_t)((uint64_t)(tick_x + step_ticks) * 370u / total_ticks);
                int32_t xmid = (x0 + x1) / 2;

                lv_draw_triangle_dsc_t tri_dsc;
                lv_draw_triangle_dsc_init(&tri_dsc);
                tri_dsc.base.layer = layer;
                tri_dsc.p[0].x = x0;   tri_dsc.p[0].y = cy + THERAPY_GUIDE_LOW_Y;   /* 基线左 */
                tri_dsc.p[1].x = xmid; tri_dsc.p[1].y = cy + THERAPY_GUIDE_HIGH_Y;  /* 峰顶 */
                tri_dsc.p[2].x = x1;   tri_dsc.p[2].y = cy + THERAPY_GUIDE_LOW_Y;   /* 基线右 */
                tri_dsc.color = lv_color_hex(FONT_GREEN_COLOR);
                tri_dsc.opa = LV_OPA_COVER;
                lv_draw_triangle(layer, &tri_dsc);
            }
            tick_x += step_ticks;
        }
    }
}
/**
 * @brief 切换治疗阶段引导波形
 * @param phase_idx 阶段序号（0=正弦波 / 1=三角形 ）
 */
void Therapy_Guide_Update(uint8_t phase_idx)
{
    if(phase_idx == 0) {
        lv_obj_add_event_cb(g_guide_cont_line, therapy_guide_draw_line_cb, LV_EVENT_DRAW_MAIN, NULL);
    } else if(phase_idx == 1) {
        lv_obj_add_event_cb(g_guide_cont, therapy_guide_draw_tri_cb, LV_EVENT_DRAW_MAIN, NULL);
    }
}
/**
 * @brief 创建评估引导波形容器（必须在曲线图之前调用，使其位于波形底层）
 * @param parent 页面容器（与曲线图同父）
 */
void Therapy_Guide_Init(lv_obj_t *parent)
{
    
    g_guide_cont = lv_obj_create(parent);
    lv_obj_set_pos(g_guide_cont, 45, 130);
    lv_obj_set_size(g_guide_cont, 370, 120);
    lv_obj_set_style_bg_opa(g_guide_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_guide_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_guide_cont, 0, 0);
    lv_obj_set_scrollbar_mode(g_guide_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(g_guide_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(g_guide_cont, therapy_guide_draw_tri_cb, LV_EVENT_DRAW_MAIN, NULL);
}
/**
 * @brief 创建评估引导波形容器（必须在曲线图之前调用，使其位于波形底层）
 * @param parent 页面容器（与曲线图同父）
 */
void Therapy_Guide_LineInit(lv_obj_t *parent)
{
    g_guide_cont_line = lv_obj_create(parent);
    lv_obj_set_pos(g_guide_cont_line, 45, 130);
    lv_obj_set_size(g_guide_cont_line, 370, 120);
    lv_obj_set_style_bg_opa(g_guide_cont_line, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_guide_cont_line, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_guide_cont_line, 0, 0);
    lv_obj_set_scrollbar_mode(g_guide_cont_line, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(g_guide_cont_line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(g_guide_cont_line, therapy_guide_draw_line_cb, LV_EVENT_DRAW_MAIN, NULL);
}
// /**
//  * @brief 切换治疗阶段引导波形（触发重绘为对应阶段的三角峰）
//  * @param phase_idx 阶段序号（0=前静息 / 1=快速收缩 / 2=持续收缩 / 3=后静息）
//  */
// void therapy_Guide_Update(uint8_t phase_idx)
// {
//     g_guide_phase = phase_idx;
//     if (g_guide_cont != NULL) {
//         lv_obj_invalidate(g_guide_cont);
//     }
// }

/* ===== [我们] 子页面 stub（B2~B5 实现）：占位 + 返回 =====
 * 非 static：B5-C 四宫格保养/障碍/凯格尔三格占位复用 */
extern lv_font_t lv_font_simhei_24px;
static void stub_back_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED ||
        (code == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC)) {
        Goto_MenuPage();
    }
}
void Show_StubPage(const char* title)
{
    ESP_LOGW(TAG, "%s: not implemented (stub)", title);
    lv_obj_t* cont = Create_Obj(g_Ui.page_container, 480, 290, 0xffffff, 1);
    lv_obj_center(cont);
    lv_obj_t* lab = lv_label_create(cont);
    lv_label_set_text_fmt(lab, "%s 功能开发中", title);
    lv_obj_set_style_text_font(lab, &lv_font_simhei_24px, LV_PART_MAIN);
    lv_obj_set_style_text_color(lab, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_center(lab);
    lv_obj_t* back_btn = lv_button_create(cont);
    lv_obj_set_size(back_btn, 160, 48);
    lv_obj_align(back_btn, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x68D0C3), LV_PART_MAIN);
    lv_obj_t* back_lab = lv_label_create(back_btn);
    lv_label_set_text(back_lab, "返回菜单");
    lv_obj_set_style_text_font(back_lab, &lv_font_simhei_24px, LV_PART_MAIN);
    lv_obj_center(back_lab);
    lv_obj_add_event_cb(back_btn, stub_back_cb, LV_EVENT_ALL, NULL);
    lv_group_t *g = app_keypad_get_group();
    if (g) { lv_group_add_obj(g, back_btn); lv_group_focus_obj(back_btn); }
}

Menu_Widget_t Menu_Widget;

/**
 * @brief 菜单页面创建入口
 * @param page_id
 */
void Menu_Page_Load(Menu_PageID_t page_id)
{
    Page_Clean();
    switch (page_id) {
    case PAGE_PELFLO_ASS:
        PelFlo_Ass_ui();
        break;
    case PAGE_PELFLO_THE:
        PelFlo_The_ui();
        break;
    case PAGE_POSREH:
        PosReh_Page_ui();
        break;
    case PAGE_SETTING:
        PelFlo_Set_ui();
        break;
    case PAGE_MENU:
        Menu_ui(g_Ui.page_container);
        break;
    default:
        break;
    }
}
void Menu_ui(lv_obj_t* parent)
{

    lv_obj_t* page_cont = Create_Obj(parent,480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_center(page_cont);
    lv_obj_set_style_radius(page_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(page_cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */

    Menu_widget(&Menu_Widget, page_cont);
}

/**
 * @brief 创建 4 个菜单按钮（评估/治疗/产后修复/设置）
 *        路径为同事原装 Windows 路径（由 ui_port/app_fs 'C' 盘翻译到 SPIFFS）
 */
static void Menu_widget(Menu_Widget_t* widget, lv_obj_t* page_cont)
{

    /*盆底评估按钮*/
    widget->PelFlo_Ass_Btn = Create_ImaButton(page_cont, 200, 110, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/green.bin");
    lv_obj_set_pos(widget->PelFlo_Ass_Btn, 21, 10);
    /*盆底评估容器照片*/
    lv_obj_t* PelFlo_Ass_Img = Creat_Image(widget->PelFlo_Ass_Btn, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/PDPG.bin");
    lv_obj_align(PelFlo_Ass_Img, LV_ALIGN_CENTER, 0, -15);
    lv_obj_add_event_cb(widget->PelFlo_Ass_Btn, Menu_button_event_cb, LV_EVENT_ALL, widget);//LV_EVENT_ALL
    /*盆底评估标签*/
    lv_obj_t* PelFlo_Ass_Lab = Creat_Label(widget->PelFlo_Ass_Btn, "盆底评估", basic_widget.Chinise_Font_Btn);
    lv_obj_align(PelFlo_Ass_Lab, LV_ALIGN_BOTTOM_MID, 0, 5);
    /*创建选中效果图片*/
    lv_obj_t* seleckImg1 = Creat_Image(widget->PelFlo_Ass_Btn, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/seleck.bin");
    lv_obj_set_pos(seleckImg1, 5, 5);
    widget->Ass_seleckImg = seleckImg1;


    /*盆底治疗按钮*/
    widget->PelFlo_The_Btn = Create_ImaButton(page_cont, 200, 110, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/red.bin");
    lv_obj_set_pos( widget->PelFlo_The_Btn, 244, 10);
    /*盆底治疗容器照片*/
    lv_obj_t* PelFlo_The_Img = Creat_Image( widget->PelFlo_The_Btn, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/PDZL.bin");
    lv_obj_align(PelFlo_The_Img, LV_ALIGN_CENTER, 0, -15); /*  lv_obj_add_flag(widget->PelFlo_The_Btn, LV_OBJ_FLAG_CHECKABLE);*/
    lv_obj_add_event_cb(widget->PelFlo_The_Btn, Menu_button_event_cb, LV_EVENT_ALL, widget);//LV_EVENT_ALL
    /*盆底治疗标签*/
    lv_obj_t* PelFlo_The_Lab = Creat_Label( widget->PelFlo_The_Btn, "盆底治疗", basic_widget.Chinise_Font_Btn);
    lv_obj_align(PelFlo_The_Lab, LV_ALIGN_BOTTOM_MID, 0, 5);
    /*创建选中效果图片*/
    lv_obj_t* seleckImg2 = Creat_Image( widget->PelFlo_The_Btn, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/seleck.bin");
    lv_obj_set_pos(seleckImg2, 5, 5);
    widget->The_seleckImg = seleckImg2;


    /*产后修复按钮*/
    widget->PostRec_Btn = Create_ImaButton(page_cont, 200, 110, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/yellow.bin");
    lv_obj_set_pos(widget->PostRec_Btn, 21, 137);
    /*产后修复容器照片*/
    lv_obj_t* PostRec_Img = Creat_Image( widget->PostRec_Btn,  "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/CHXF.bin");
    lv_obj_align(PostRec_Img, LV_ALIGN_CENTER, 0, -15);
    lv_obj_add_event_cb(widget->PostRec_Btn, Menu_button_event_cb, LV_EVENT_ALL, widget);//LV_EVENT_ALL
    /*产后修复标签*/
    lv_obj_t* PostRec_Lab = Creat_Label( widget->PostRec_Btn, "产后康复", basic_widget.Chinise_Font_Btn);
    lv_obj_align(PostRec_Lab, LV_ALIGN_BOTTOM_MID, 0, 5);
    /*创建选中效果图片*/
    lv_obj_t* seleckImg4 = Creat_Image( widget->PostRec_Btn, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/seleck.bin");
    lv_obj_set_pos(seleckImg4, 5, 5);
    widget->PostRec_seleckImg = seleckImg4;

    /*设置按钮*/
    widget->Set_Btn = Create_ImaButton(page_cont, 200, 110, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/blue.bin");
    lv_obj_set_pos( widget->Set_Btn, 244, 137);
    /*设置容器照片*/
    lv_obj_t* Set_Img = Creat_Image( widget->Set_Btn,"C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/SZ.bin");
    lv_obj_align(Set_Img,LV_ALIGN_CENTER, 0, -15);
    lv_obj_add_event_cb(widget->Set_Btn, Menu_button_event_cb, LV_EVENT_ALL, widget);//LV_EVENT_ALL
    /*设置标签*/
    lv_obj_t* Set_Lab = Creat_Label( widget->Set_Btn, "设置", basic_widget.Chinise_Font_Btn);
    lv_obj_set_pos(Set_Lab, 75, 75);
    lv_obj_align(Set_Lab, LV_ALIGN_BOTTOM_MID, 0, 5);
    /*创建选中效果图片*/
    lv_obj_t* seleckImg3 = Creat_Image( widget->Set_Btn, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/seleck.bin");
    lv_obj_set_pos(seleckImg3, 5, 5);
    widget->Set_seleckImg = seleckImg3;

    /* ===== [我们] 注册按钮到 keypad group（按键导航） ===== */
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        lv_group_add_obj(g, widget->PelFlo_Ass_Btn);
        lv_group_add_obj(g, widget->PelFlo_The_Btn);
        lv_group_add_obj(g, widget->PostRec_Btn);
        lv_group_add_obj(g, widget->Set_Btn);
    }
}

/* ============================================================================
 * [我们] B4 PosReh 共享控件（同事 MenuPage 原装，移植自 EDA_EMG_LVGL_PC\MenuPage\menu_ui.c）
 *  - 菜单页子页面（PosReh ctx 的 Par_BackPage/Par_CurPage 引用）
 *  - 治疗页滚动补偿外部坐标
 *  - Warn_Window / Treat_Widget / ParSet_Widget / TreIns_Widget
 * 配套回调在 menu_ui_event_cb.c
 * ==========================================================================*/


lv_coord_t ThePage_Obj_origin_y = 0;       //模糊遮罩起始位置
lv_coord_t ThePage_Obj_origin_y2 = 0;      //模糊遮罩起始位置
lv_coord_t ThePage_HisBtn_origin_y = 0;    //查看历史按钮起始位置
lv_coord_t ThePage_StrBtn_origin_y = 0;    //开始治疗按钮起始位置

/**
 * @brief 提示窗（无按钮，纯提示）—— 对齐 PC 新版 Warn_Window(Window_Data_t*)
 */
void Warn_Window(Window_Data_t* data)
{
    lv_obj_t* msgbox = lv_msgbox_create(NULL);
    lv_obj_set_size(msgbox, 300, 50);                                                      /* 设置大小 */
    lv_obj_align(msgbox, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_border_width(msgbox, 0, LV_STATE_DEFAULT);                             /* 去除边框 */
    lv_obj_set_style_bg_color(msgbox, lv_color_hex(0x5e5f62), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(msgbox, 150, LV_PART_MAIN);
    data->Msgbox = msgbox;

    /*设置消息框内容*/
    lv_obj_t* content = lv_msgbox_get_content(msgbox);
    lv_obj_set_style_pad_top(content, 15, LV_STATE_DEFAULT);                /* 设置顶部填充 */
    lv_obj_set_style_pad_left(content, 35, LV_STATE_DEFAULT);               /* 设置左侧填充 */

    lv_obj_t* text = Creat_Label(content, "腹部参与度过高，请放松您的腹部", basic_widget.Chinise_Font_Btn);
    lv_obj_set_style_text_color(text, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
    lv_obj_center(text);
}

/**
 * @brief 是否返回弹窗（是/否）—— 对齐 PC 新版 Back_Window(Window_Data_t*, txt)
 *        [我们] 事件绑定在调用处（PosReh_*_ShowConfirm 键盘化时绑 Back_Btn_Event_cb）
 */
void Back_Window(Window_Data_t* data, const char* txt)
{
    lv_obj_t* msgbox = lv_msgbox_create(NULL);
    lv_obj_set_size(msgbox, 160, 70);                                                      /* 设置大小 */
    lv_obj_center(msgbox);                                                                  /* 设置位置 */
    lv_obj_set_style_border_width(msgbox, 0, LV_STATE_DEFAULT);                             /* 去除边框 */
    lv_obj_set_style_bg_color(msgbox, lv_color_hex(0x5e5f62), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(msgbox, 150, LV_PART_MAIN);
    data->Msgbox = msgbox;

    /*设置消息框内容*/
    lv_obj_t* content = lv_msgbox_get_content(msgbox);
    lv_obj_set_style_pad_top(content, 15, LV_STATE_DEFAULT);                /* 设置顶部填充 */
    lv_obj_set_style_pad_left(content, 35, LV_STATE_DEFAULT);               /* 设置左侧填充 */

    lv_obj_t* text = Creat_Label(content, (char*)txt, basic_widget.Chinise_Font_Btn);
    lv_obj_set_style_text_color(text, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    /*设置底部按钮*/
    lv_obj_t* btn = lv_msgbox_add_footer_button(msgbox, "是");
    lv_obj_set_style_text_color(btn, lv_color_hex(FONT_WHITE_COLOR), 0);
    lv_obj_set_style_text_font(btn, basic_widget.Chinise_Font_Btn, 0);
    lv_obj_set_style_bg_opa(btn, 0, LV_PART_MAIN);                          /* 设置按钮背景透明度 */
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);                    /* 去除按钮阴影 */
    data->Yes_Btn = btn;

    lv_obj_t* btn2 = lv_msgbox_add_footer_button(msgbox, "否");
    lv_obj_set_style_text_color(btn2, lv_color_hex(FONT_WHITE_COLOR), 0);
    lv_obj_set_style_text_font(btn2, basic_widget.Chinise_Font_Btn, 0);
    lv_obj_set_style_bg_opa(btn2, 0, LV_PART_MAIN);                          /* 设置按钮背景透明度 */
    lv_obj_set_style_shadow_width(btn2, 0, LV_PART_MAIN);                    /* 去除按钮阴影 */
    data->No_Btn = btn2;
    lv_obj_t* footer = lv_msgbox_get_footer(msgbox);
    lv_obj_set_style_pad_bottom(footer, 10, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(footer, 50, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(footer, 50, LV_STATE_DEFAULT);
    lv_obj_set_flex_grow(footer, 1);
}

/**
 * @brief 电刺激治疗界面
 */
void Treat_Widget(lv_obj_t* page_cont, Treat_Timer_Ctx_t* data)
{
   
    /*脉宽图标*/
    lv_obj_t* pulse_img = Creat_Image(page_cont, PULSE_ICON);
    lv_obj_set_pos(pulse_img, 30, 20);
    lv_obj_set_style_image_recolor_opa(pulse_img, 255, 0);
    lv_obj_set_style_image_recolor(pulse_img, lv_color_hex(data->treat_data.Color), 0);
    /*脉宽标签*/
    lv_obj_t* Label1 = Creat_Label(page_cont, "脉宽", basic_widget.Chinise_Font_Btn);
    lv_obj_set_pos(Label1, 60, 25);
    lv_obj_set_size(Label1, 50, 32);
    lv_obj_set_style_text_color(Label1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*脉宽容器*/
    lv_obj_t* Obj = Create_Obj(page_cont, 75, 40, FONT_BLUE_COLOR, 0);
    lv_obj_set_style_radius(Obj, 10, LV_PART_MAIN);
    lv_obj_set_pos(Obj, 30, 60);
    lv_obj_add_style(Obj, &style_gradient2, LV_PART_MAIN);
    /*参数*/
    lv_obj_t* param = Creat_Label(Obj, "", basic_widget.Chinise_Font_Btn);
    lv_obj_set_pos(param, 5, -2);
    lv_obj_set_size(param, 50, 20);
    lv_label_set_text_fmt(param, "%duS", (int)data->treat_data.Value_Pulse);
    lv_obj_set_style_text_color(param, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_Pulse = param;   /* [我们] 阶段C：当前步骤脉宽标签（治疗中随阶段切换） */

    /*频率图标*/
    lv_obj_t* pre_img = Creat_Image(page_cont, FREQUENCY_ICON);
    lv_obj_set_pos(pre_img, 30, 110);
    lv_obj_set_style_image_recolor_opa(pre_img, 255, 0);
    lv_obj_set_style_image_recolor(pre_img, lv_color_hex(data->treat_data.Color), 0);
    /*标签*/
    lv_obj_t* Label2 = Creat_Label(page_cont, "频率", basic_widget.Chinise_Font_Btn);
    lv_obj_set_pos(Label2, 60, 115);
    lv_obj_set_size(Label2, 50, 32);
    lv_obj_set_style_text_color(Label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*频率容器*/
    lv_obj_t* Obj1 = Create_Obj(page_cont, 75, 40, FONT_BLUE_COLOR, 0);
    lv_obj_set_style_radius(Obj1, 10, LV_PART_MAIN);
    lv_obj_set_pos(Obj1, 30, 150);
    lv_obj_add_style(Obj1, &style_gradient2, LV_PART_MAIN);
    /*参数*/
    lv_obj_t* param2 = Creat_Label(Obj1, "", basic_widget.Chinise_Font_Btn);
    lv_obj_set_pos(param2, 15, -2);
    lv_obj_set_size(param2, 50, 20);
    lv_label_set_text_fmt(param2, "%dHZ", (int)data->treat_data.Value_Freq);
    lv_obj_set_style_text_color(param2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_Freq = param2;   /* [我们] 阶段C：当前步骤频率标签（治疗中随阶段切换） */

    /*时间显示图标*/
    lv_obj_t* time_img = Creat_Image(page_cont, TIME_ICON);
    lv_obj_set_pos(time_img, 85, -30);
    /*剩余时间*/
    lv_obj_t* time = Creat_Label(page_cont, "", basic_widget.Chinese_Font_30);
    lv_label_set_text_fmt(time, "%02d:%02d:%02d",
                                (int)(data->total_sec / 60000),
                                (int)((data->total_sec % 60000) / 1000),
                                (int)((data->total_sec % 1000) / 10));
    lv_obj_align(time, LV_ALIGN_CENTER, -10, -30);
    lv_obj_set_style_text_color(time, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_RemTime = time;
    /*剩余时间标签*/
    lv_obj_t* time_label = Creat_Label(page_cont, "剩余时间", basic_widget.Chinise_Font_Unit);
    lv_obj_align(time_label, LV_ALIGN_CENTER, -10, 0);
    lv_obj_set_style_text_color(time_label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_Hint = time_label;
    /*强度标签*/
    lv_obj_t* Intens_label = Creat_Label(page_cont, "强度", basic_widget.Chinise_Font_Btn);
    lv_obj_set_pos(Intens_label, 375, 25);
    lv_obj_set_style_text_color(Intens_label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*强度容器*/
    lv_obj_t* Obj2 = Create_Obj(page_cont, 95, 135, FONT_BLUE_COLOR, 0);
    lv_obj_set_style_radius(Obj2, 12, LV_PART_MAIN);
    lv_obj_set_pos(Obj2, 345, 50);
    lv_obj_add_style(Obj2, &style_gradient, LV_PART_MAIN);
    lv_obj_add_style(Obj2, &style_shadow, LV_PART_MAIN);

    /*按钮加*/
    lv_obj_t* Btn = Create_Button(Obj2, 75, 30, 0xffffff, 10, 0);
    lv_obj_set_pos(Btn, 0, 3);
    lv_obj_add_style(Btn, &style_gradient2, LV_PART_MAIN);
    lv_obj_add_style(Btn, &style_shadow, LV_PART_MAIN);
    data->treat_data.Plu_Btn = Btn;
    /*按钮加图标*/
    lv_obj_t* plu_img = Creat_Image(Btn, PLUUP_UNSEL_ICON);
    lv_obj_center(plu_img);
    lv_obj_set_style_image_recolor_opa(plu_img, 255, 0);
    lv_obj_set_style_image_recolor(plu_img, lv_color_hex(FONT_GRAY_COLOR), 0);
    data->treat_data.Plu_src = plu_img;
    lv_obj_add_event_cb(data->treat_data.Plu_Btn, Treat_Widget_Event_cb, LV_EVENT_ALL, data);

    /*按钮减*/
    lv_obj_t* Btn2 = Create_Button(Obj2, 75, 30, 0xffffff, 10, 0);
    lv_obj_set_pos(Btn2, 0, 85);
    lv_obj_add_style(Btn2, &style_gradient2, LV_PART_MAIN);
    lv_obj_add_style(Btn2, &style_shadow, LV_PART_MAIN);
    data->treat_data.Min_Btn = Btn2;
    /*按钮减图标*/
    lv_obj_t* min_img = Creat_Image(Btn2, MINDOWN_UNSEL_ICON);
    lv_obj_center(min_img);
    lv_obj_set_style_image_recolor_opa(min_img, 255, 0);
    lv_obj_set_style_image_recolor(min_img, lv_color_hex(FONT_GRAY_COLOR), 0);
    data->treat_data.Min_src = min_img;
    lv_obj_add_event_cb(data->treat_data.Min_Btn, Treat_Widget_Event_cb, LV_EVENT_ALL, data);

    /*强度值显示（[我们] B5-A 对齐 PC 新版：动态绑定强度初值，去硬编码 "3"）*/
    lv_obj_t* Intens = Creat_Label(Obj2, "", basic_widget.Chinise_Font_Title);
    lv_obj_set_pos(Intens, 32, 50);
    lv_obj_set_style_text_color(Intens, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_Intens = Intens;
    lv_label_set_text_fmt(Intens, "%d", (int)data->treat_data.Intens_Value);

    /*暂停按钮*/
    lv_obj_t* Btn3 = Create_Button(page_cont, 100, 40, 0xf6f6f6, 20, 0);
    lv_obj_set_pos(Btn3, 40, 220);
    lv_obj_set_style_shadow_opa(Btn3, 0, LV_PART_MAIN);
    data->treat_data.Pause_Btn = Btn3;
    /*暂停按钮图标*/
    lv_obj_t* pause_img = Creat_Image(Btn3, PAUSE_UNSEL_ICON);
    lv_obj_align(pause_img, LV_ALIGN_CENTER, -12, 0);
    lv_obj_set_style_image_recolor_opa(pause_img, 255, 0);
    lv_obj_set_style_image_recolor(pause_img, lv_color_hex(FONT_GRAY_COLOR), 0);
    data->treat_data.Pause_src = pause_img;
    /*暂停标签*/
    lv_obj_t* pause = Creat_Label(Btn3, "暂停", basic_widget.Chinise_Font_Btn);
    lv_obj_align(pause, LV_ALIGN_CENTER, 12, 0);
    lv_obj_set_style_text_color(pause, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_Pause = pause;
    /*开始按钮*/
    lv_obj_t* Btn4 = Create_Button(page_cont, 100, 40, 0xf6f6f6, 20, 0);
    lv_obj_set_pos(Btn4, 165, 220);
    lv_obj_set_style_shadow_opa(Btn4, 0, LV_PART_MAIN);
    data->treat_data.Start_Btn = Btn4;
    /*开始按钮图标*/
    lv_obj_t* start_img = Creat_Image(Btn4, START_UNSEL_ICON);
    lv_obj_align(start_img, LV_ALIGN_CENTER, -12, 0);
    lv_obj_set_style_image_recolor_opa(start_img, 255, 0);
    lv_obj_set_style_image_recolor(start_img, lv_color_hex(FONT_GRAY_COLOR), 0);
    data->treat_data.Start_src = start_img;
    /*开始标签*/
    lv_obj_t* start = Creat_Label(Btn4, "开始", basic_widget.Chinise_Font_Btn);
    lv_obj_align(start, LV_ALIGN_CENTER, 12, 0);
    lv_obj_set_style_text_color(start, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_Start = start;
    /*返回按钮*/
    lv_obj_t* Btn5 = Create_Button(page_cont, 100, 40, 0xf6f6f6, 20, 0);
    lv_obj_set_pos(Btn5, 290, 220);
    lv_obj_set_style_shadow_opa(Btn5, 0, LV_PART_MAIN);
    data->treat_data.Back_Btn = Btn5;
    /*返回按钮图标*/
    lv_obj_t* back_img = Creat_Image(Btn5, BACK_UNSEL_ICON);
    lv_obj_align(back_img, LV_ALIGN_CENTER, -14, 0);
    lv_obj_set_style_image_recolor_opa(back_img, 255, 0);
    lv_obj_set_style_image_recolor(back_img, lv_color_hex(FONT_GRAY_COLOR), 0);
    data->treat_data.Back_src = back_img;
    /*返回标签*/
    lv_obj_t* back = Creat_Label(Btn5, "返回", basic_widget.Chinise_Font_Btn);
    lv_obj_align(back, LV_ALIGN_CENTER, 12, 0);
    lv_obj_set_style_text_color(back, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_Back = back;
    /*电刺激阶段标签*/
    lv_obj_t* Label_stage = Creat_Label(page_cont, "当前阶段:1", basic_widget.Chinise_Font_Btn);
    lv_obj_align(Label_stage, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_color(Label_stage, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_stage_stime = Label_stage;

    lv_obj_add_event_cb(data->treat_data.Pause_Btn, Treat_Widget_Event_cb, LV_EVENT_FOCUSED, data);
    lv_obj_add_event_cb(data->treat_data.Start_Btn, Treat_Widget_Event_cb, LV_EVENT_FOCUSED, data);
    lv_obj_add_event_cb(data->treat_data.Back_Btn, Treat_Widget_Event_cb, LV_EVENT_FOCUSED, data);
}

/**
 * @brief 电刺激参数设置界面
 */
void ParSet_Widget(lv_obj_t* page_cont, Param_Data_t* data)
{
    /*脉宽容器*/
    lv_obj_t* Obj = Create_Obj(page_cont, 96, 69, FONT_BLUE_COLOR, 0);
    lv_obj_set_pos(Obj, 30, 10);
    /*图标*/
    lv_obj_t* pulse_img = Creat_Image(Obj, PULSE_ICON);
    lv_obj_set_pos(pulse_img, 5, 5);
    lv_obj_set_style_image_recolor_opa(pulse_img, 255, 0);
    lv_obj_set_style_image_recolor(pulse_img, lv_color_hex(data->Slder_Color), 0);
    /*标签*/
    lv_obj_t* Label1 = Creat_Label(Obj, "脉宽", basic_widget.Chinise_Font_Btn);
    lv_obj_set_pos(Label1, 35, 10);
    lv_obj_set_size(Label1, 50, 32);
    lv_obj_set_style_text_color(Label1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*参数*/
    lv_obj_t* param = Creat_Label(Obj, "", basic_widget.Chinise_Font_Btn);
    /* [我们] -Werror=format：uint32_t=unsigned long，%d → (int) */
    lv_label_set_text_fmt(param, "%duS", (int)data->Value_Pulse);
    lv_obj_set_pos(param, 15, 33);
    lv_obj_set_size(param, 50, 20);
    lv_obj_set_style_text_color(param, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->Label_Pulse = param;
    /*设置容器*/
    lv_obj_t* Obj2 = Create_Obj(page_cont, 120, 69, FONT_BLUE_COLOR, 0);
    lv_obj_set_pos(Obj2, 183, 10);
    /*图标*/
    lv_obj_t* set_img = Creat_Image(Obj2, SET_STAGE_ICON);
    lv_obj_set_pos(set_img, 5, 5);
    lv_obj_set_style_image_recolor_opa(set_img, 255, 0);
    lv_obj_set_style_image_recolor(set_img, lv_color_hex(data->Slder_Color), 0);
    /*标签*/
    lv_obj_t* Label2 = Creat_Label(Obj2, "设置阶段", basic_widget.Chinise_Font_Btn);
    lv_obj_set_pos(Label2, 35, 10);
    lv_obj_set_size(Label2, 80, 32);
    lv_obj_set_style_text_color(Label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*参数*/
     lv_obj_t* param2 = NULL;
    if(data->stage_stim == 2){
        param2 = Creat_Label(Obj2, "1/2", basic_widget.Chinise_Font_Btn);
    } else {
        param2 = Creat_Label(Obj2, "1", basic_widget.Chinise_Font_Btn);
    }
    
    
    lv_obj_set_pos(param2, 45, 33);
    lv_obj_set_size(param2, 50, 20);
    lv_obj_set_style_text_color(param2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->Label_Stage = param2;
    data->Value_Stage = 1;      //默认为第一阶段
    /*频率容器*/
    lv_obj_t* Obj3 = Create_Obj(page_cont, 96, 69, FONT_BLUE_COLOR, 0);
    lv_obj_set_pos(Obj3, 341, 10);
    /*图标*/
    lv_obj_t* pre_img = Creat_Image(Obj3, FREQUENCY_ICON);
    lv_obj_set_pos(pre_img, 5, 5);
    lv_obj_set_style_image_recolor_opa(pre_img, 255, 0);
    lv_obj_set_style_image_recolor(pre_img, lv_color_hex(data->Slder_Color), 0);
    /*标签*/
    lv_obj_t* Label3 = Creat_Label(Obj3, "频率", basic_widget.Chinise_Font_Btn);
    lv_obj_set_pos(Label3, 35, 10);
    lv_obj_set_size(Label3, 50, 32);
    lv_obj_set_style_text_color(Label3, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*参数*/
    lv_obj_t* param3 = Creat_Label(Obj3, "", basic_widget.Chinise_Font_Btn);
    /* [我们] -Werror=format：uint32_t=unsigned long，%d → (int) */
    lv_label_set_text_fmt(param3, "%dHz", (int)data->Value_Freq);
    lv_obj_set_pos(param3, 25, 33);
    lv_obj_set_size(param3, 50, 20);
    lv_obj_set_style_text_color(param3, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->Label_Freq = param3;

    lv_obj_t* Obj4 = Create_Obj(page_cont, 330, 4, 0xebecf0, 1);
    lv_obj_set_pos(Obj4, 58, 112);
    /*创建滑动条*/
    lv_obj_t* slider = Create_Sliders(page_cont, data->Slder_Color, 90);
    lv_obj_set_pos(slider, 58, 107);
    data->Slider = slider;
    lv_obj_add_event_cb(data->Slider, Child_Slider_Event_cb, LV_EVENT_ALL, data);    //将加按钮添加任何事件回调
    /* [我们] 滑条初值同步下移到 Label_Intens 赋值之后（见下），此处只建 slider */
    /*减号按钮*/
    lv_obj_t* Btn1 = Create_Button(page_cont, 85, 40, 0xE6D6DA, 30, 0);
    lv_obj_set_pos(Btn1, 65, 150);
    lv_obj_add_style(Btn1, &style_gradient, LV_PART_MAIN);
    lv_obj_add_style(Btn1, &style_shadow, LV_PART_MAIN);
    lv_obj_t* min_img = Creat_Image(Btn1, MIN_SEL_ICON);
    lv_obj_center(min_img);
    lv_obj_set_style_image_recolor_opa(min_img, 255, 0);
    lv_obj_set_style_image_recolor(min_img, lv_color_hex(FONT_GRAY_COLOR), 0);
    data->Min_Btn = Btn1;
    data->Min_src = min_img;
    lv_obj_add_event_cb(data->Min_Btn, Child_Slider_Event_cb, LV_EVENT_ALL, data);    //将加按钮添加任何事件回调

    /*加号按钮*/
    lv_obj_t* Btn2 = Create_Button(page_cont, 85, 40, 0xE6D6DA, 30, 0);
    lv_obj_set_pos(Btn2, 310, 150);
    lv_obj_add_style(Btn2, &style_gradient, LV_PART_MAIN);
    lv_obj_add_style(Btn2, &style_shadow, LV_PART_MAIN);
    lv_obj_t* plu_img = Creat_Image(Btn2, PLU_SEL_ICON);
    lv_obj_center(plu_img);
    lv_obj_set_style_image_recolor_opa(plu_img, 255, 0);
    lv_obj_set_style_image_recolor(plu_img, lv_color_hex(FONT_GRAY_COLOR), 0);
    data->Plu_Btn = Btn2;
    data->Plu_src = plu_img;
    lv_obj_add_event_cb(data->Plu_Btn, Child_Slider_Event_cb, LV_EVENT_ALL, data);    //将加按钮添加任何事件回调
    /*强度标签*/
    lv_obj_t* Label4 = Creat_Label(page_cont, "0", basic_widget.Chinise_Font_Title);
    lv_obj_set_pos(Label4, 215, 165);
    lv_obj_set_style_text_color(Label4, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->Label_Intens = Label4;
    /* [我们] ⚠️ 滑条初值同步必须放在 Label_Intens 赋值之后：
     * LVGL9.5 lv_slider_set_value(ANIM_OFF) 不发 VALUE_CHANGED（lv_bar.c 确认），
     * 需手动同步强度标签；若提前，data->Label_Intens 为 NULL → lv_label_set_text_fmt 崩溃
     * （坑：产后页点 单次/10次治疗 → 参数设置页进不去）。 */
    int32_t _init_intens = (data->Value_Stage == 2) ? data->Value_Intens2 : data->Value_Intens1;
    lv_slider_set_value(data->Slider, _init_intens, LV_ANIM_OFF);
    lv_label_set_text_fmt(data->Label_Intens, "%d", (int)_init_intens);
    /*设置按钮*/
    lv_obj_t* Btn3 = Create_Button(page_cont, 202, 40, 0xffffff, 30, 0);
    lv_obj_set_pos(Btn3, 120, 210);
    lv_obj_t* Labe5 = NULL;
    if(data->stage_stim == 2){
        Labe5 = Creat_Label(Btn3, data->Text_Set1, basic_widget.Chinise_Font_Btn);
    } else {
       Labe5 = Creat_Label(Btn3, data->Text_Set2, basic_widget.Chinise_Font_Btn);
    }
    lv_obj_center(Labe5);
    lv_obj_set_style_text_color(Labe5, lv_color_hex(data->Slder_Color), LV_PART_MAIN);
    data->Label_Set = Labe5;
    data->Set_Btn = Btn3;
    /* [我们] B5-A 对齐 PC 新版：Param_Data_t 已删 Page_Next，删除 user_data 跳转（PC 端已注释） */
    lv_obj_add_event_cb(data->Set_Btn, Child_Slider_Event_cb, LV_EVENT_FOCUSED, data);    //将加按钮添加聚焦事件回调
}

/**
 * @brief 阶段疗程说明界面
 */
void TreIns_Widget(lv_obj_t* page_cont, TreIns_Data_t* data)
{
    /*说明框*/
    lv_obj_t* Obj = Create_Obj(page_cont, data->Obj_Wid, data->Obj_Hig, 0xffffff, 0);
    lv_obj_set_pos(Obj, 25, 0);
    /*添加样式*/
    lv_obj_add_style(Obj, &style_gradient, LV_PART_MAIN);    // 中层：渐变
    lv_obj_add_style(Obj, &style_shadow, LV_PART_MAIN);      // 底层：阴影
    

    /*标题*/
    lv_obj_t* title1 = Creat_Label(Obj, data->Title, basic_widget.Chinise_Font_Title);
    lv_obj_set_pos(title1, 14, 10);
    lv_obj_set_style_text_color(title1, lv_color_hex(data->Title_Color), LV_PART_MAIN);
    lv_obj_t* title12 = Creat_Label(Obj, data->Title_time, basic_widget.Chinise_Font_Title);
    lv_obj_set_pos(title12, 85, 10);
    lv_obj_set_style_text_color(title12, lv_color_hex(data->Title_Color), LV_PART_MAIN);
    /*正文*/
    lv_obj_t* label = Creat_Label(Obj, "·", basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(label, 20, 45);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text = Creat_TextLabel(Obj, 350, 85, data->Text, basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(text, 30, 35);
    lv_obj_set_style_text_color(text, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_set_style_text_line_space(text, 10, 0);          // 行间距 10px

    /*线材接线图*/
    lv_obj_t* Obj_img = Create_Obj(page_cont, 404, 300, 0xffffff, 0);
    lv_obj_align_to(Obj_img, Obj, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_style_radius(Obj_img, 15, LV_PART_MAIN);
    lv_obj_set_style_border_width(Obj_img, 1, LV_PART_MAIN);
    lv_obj_set_style_border_opa(Obj_img, 255, LV_PART_MAIN);
    lv_obj_set_style_border_color(Obj_img, lv_color_hex(data->Title_Color), LV_PART_MAIN);
    lv_obj_clear_flag(Obj_img, LV_OBJ_FLAG_SCROLLABLE);           /*禁用滑动条*/
    data->Obj_Img = Obj_img;
    /*接线图*/
    lv_obj_t* Img = Creat_Image(Obj_img, data->src);
    lv_obj_set_pos(Img, 0, 0);

    /*底部模糊图层*/
    lv_obj_t* Obj2 = Create_Obj(page_cont, 480, 60, 0x2195f6, 0);
    lv_obj_set_pos(Obj2, -8, 225);
    lv_obj_set_style_border_width(Obj2, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(Obj2, 120, LV_PART_MAIN);
    lv_obj_clear_flag(Obj2, LV_OBJ_FLAG_SCROLLABLE);           /*禁用滑动条*/
    ThePage_Obj_origin_y = 225;                     //按钮起始位置
    lv_obj_add_event_cb(page_cont, ThePage_Obj_scroll_cb, LV_EVENT_SCROLL, Obj2);

    /*查看历史按钮*/
    lv_obj_t* Btn1 = Create_Button(page_cont, 110, 40, 0xffffff, 30, 0);
    lv_obj_set_pos(Btn1, 195, 200);
    lv_obj_add_style(Btn1, &style_RedShadow, LV_PART_MAIN);
    data->History_Btn = Btn1;
    /* [我们] B5-A 对齐 PC 新版：TreIns_Data_t 已删 Page_His，删除 user_data 跳转（PC 端已注释） */
    lv_obj_add_event_cb(Btn1, TreIns_Widget_event_cb, LV_EVENT_FOCUSED, data);              //将按钮添加到事件

    lv_obj_t* Label = Creat_Label(Btn1, data->Label_Btn1, basic_widget.Chinise_Font_Btn);
    lv_obj_center(Label);
    lv_obj_set_style_text_color(Label, lv_color_hex(data->Title_Color), LV_PART_MAIN);

    ThePage_HisBtn_origin_y = 200;          //按钮起始位置
    lv_obj_add_event_cb(page_cont, ThePage_HisBtn_scroll_cb, LV_EVENT_SCROLL, Btn1);  //将按钮添加到页面滚动事件

    /*开始评估按钮*/
    lv_obj_t* Btn2 = Create_Button(page_cont, 110, 40, data->Title_Color, 30, 0);
    lv_obj_set_pos(Btn2, 324, 200);
    lv_obj_add_style(Btn2, &style_BlueShadow, LV_PART_MAIN);
    data->Start_Btn = Btn2;
    /* [我们] B5-A 对齐 PC 新版：TreIns_Data_t 已删 Page_Start，删除 user_data 跳转（PC 端已注释） */
    lv_obj_add_event_cb(Btn2, TreIns_Widget_event_cb, LV_EVENT_FOCUSED, data);              //将按钮添加到事件
    lv_obj_t* Labe2 = Creat_Label(Btn2,data->Label_Btn2, basic_widget.Chinise_Font_Btn);
    lv_obj_center(Labe2);
    lv_obj_set_style_text_color(Labe2, lv_color_hex(0xffffff), LV_PART_MAIN);

    ThePage_StrBtn_origin_y = 200;                     //按钮起始位置
    lv_obj_add_event_cb(page_cont, ThePage_StrBtn_scroll_cb, LV_EVENT_SCROLL, Btn2);  //将按钮添加到页面滚动事件
    
}

/* ============================================================================
 * [我们] B5-B 治疗页新控件（同事 MenuPage 原装，移植自 EDA_EMG_LVGL_PC\MenuPage\menu_ui.c）
 *  - draw_pressure_excel / Creat_Chart / Chart_widget  （曲线图页）
 *  - Select_Widget2 / Stage_Widget2                    （疗程网格页，Flex 换行）
 *  - Instr_Widget                                      （说明页）
 *  配套图表事件在 menu_ui_event_cb.c；B5-C 才接入调用点
 * ==========================================================================*/

/**
 * @brief 创建曲线表格（刻度虚线网格）
 */
static void draw_pressure_excel(lv_obj_t* cont)
{
    static lv_point_precise_t line_1[] = { {0, 0},{400, 0} };
    static lv_point_precise_t line_2[] = { {0, 15},{0, 164} };
    (void)line_2; /* [我们] PC 遗留未用，防 unused 告警 */
    /*横线*/
    for (uint8_t i = 0; i < 5; i++) {
        lv_obj_t* line1 = creat_DottedLine(cont, 370, 6, line_1, 2);
        lv_obj_set_pos(line1, 45, 249 - (i * 29));
        lv_obj_clear_flag(line1, LV_OBJ_FLAG_SCROLLABLE);                 /*禁用滑动条*/
    }
    lv_obj_t* text1 = Creat_Label(cont, "0", basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(text1, 25, 238);
    lv_obj_set_style_text_color(text1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text2 = Creat_Label(cont, "25", basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(text2, 20, 211);
    lv_obj_set_style_text_color(text2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text3 = Creat_Label(cont, "50", basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(text3, 20, 184);
    lv_obj_set_style_text_color(text3, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text4 = Creat_Label(cont, "75", basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(text4, 20, 157);
    lv_obj_set_style_text_color(text4, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text5 = Creat_Label(cont, "100", basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(text5, 13, 125);
    lv_obj_set_style_text_color(text5, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
}

/**
 * @brief 创建曲线图（透明背景 lv_chart + 预填充中间值）
 */
static void Creat_Chart(Chart_Data_t* widget, lv_obj_t* page_cont)
{
    /*样式：透明背景 只留边框（static 只初始化一次，防重复注册样式）*/
    static lv_style_t style;
    static bool style_inited = false;
    if (!style_inited) {
        style_inited = true;
        lv_style_init(&style);
        lv_style_set_radius(&style, 1);
        lv_style_set_bg_opa(&style, 0);
        lv_style_set_border_width(&style, 0);
    }

    draw_pressure_excel(page_cont);

    /*创建曲线表*/
    lv_obj_t* chart = lv_chart_create(page_cont);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);   /*Show lines and points too*/
    lv_obj_set_pos(chart, 45, 130);
    lv_obj_set_size(chart, 370, 120);
    lv_obj_add_style(chart, &style, LV_PART_MAIN);
    lv_obj_set_style_pad_all(chart, 0, 0);
    lv_obj_set_style_radius(chart, 0, 0);
    lv_obj_set_style_pad_bottom(chart, 0, 0);
    lv_chart_set_div_line_count(chart, 0, 0);
    /*CIRCULAR 模式*/
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_point_count(chart, 100);

    /*设置折线线宽 设置线条端点为圆形*/
    lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS);  // 3px 宽
    lv_obj_set_style_line_rounded(chart, true, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(chart, 0, LV_PART_INDICATOR);

    /*series 1:绿线（PC 原样用红色 palette）*/
    lv_chart_series_t* ser_green = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(chart, ser_green, LV_CHART_POINT_NONE);  /* 所有点标记为无效，不画线 */
    /* 预填充初始数据（避免全 0） */
    // for (uint16_t i = 0; i < 100; i++) {
    //     lv_chart_set_next_value(chart, ser_green, 50); // 中间值
    // }

    lv_obj_add_flag(chart, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

    widget->Chart = chart;
}

/**
 * @brief 创建曲线图页面（左上盒/右上盒/腹部监控/完成标签 + 曲线表）
 */
void Chart_widget(lv_obj_t* page_cont, Chart_Data_t* widget)
{
    /*左上框*/
    lv_obj_t* Obj_left = Create_Obj(page_cont, 250, 90, 0xffffff, 0);
    lv_obj_set_pos(Obj_left, 15, 5);
    /*添加样式*/
    lv_obj_add_style(Obj_left, &style_shadow, LV_PART_MAIN);       // 底层：阴影
    lv_obj_add_style(Obj_left, &style_gradient, LV_PART_MAIN);     // 中层：渐变
    /*图标*/
    lv_obj_t* Img_left = Creat_Image(Obj_left, VAGINA_ICON);
    lv_obj_set_pos(Img_left, 10, 10);
    /*标题*/
    lv_obj_t* title1 = Creat_Label(Obj_left, "阴道", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(title1, 35, 15);
    lv_obj_set_style_text_color(title1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->Title_Vagina = title1;

    lv_obj_t* title3 = Creat_Label(Obj_left, "最大肌电位", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(title3, 25, 40);
    lv_obj_set_style_text_color(title3, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* label= Creat_Label(Obj_left, "", basic_widget.Chinise_Font_Unit);
    lv_label_set_text_fmt(label, "%.1fμA", widget->Value_MaxEMG);
    lv_obj_set_pos(label, 35, 60);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_RED_COLOR), LV_PART_MAIN);
    widget->Label_MaxEMG = label;

    lv_obj_t* title4 = Creat_Label(Obj_left, "瞬时肌电位", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(title4, 98, 40);
    lv_obj_set_style_text_color(title4, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* label2 = Creat_Label(Obj_left, "", basic_widget.Chinise_Font_Unit);
    lv_label_set_text_fmt(label2, "%.1fμA", widget->Value_InsEMG);
    lv_obj_set_pos(label2, 105, 60);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_RED_COLOR), LV_PART_MAIN);
    widget->Label_InsEMG = label2;

    lv_obj_t* title5 = Creat_Label(Obj_left, "剩余时间", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(title5, 168, 40);
    lv_obj_set_style_text_color(title5, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    lv_obj_t* label3 = Creat_Label(Obj_left, "", basic_widget.Chinise_Font_Unit);
    /* [我们] -Wformat：uint32_t=unsigned long，%d → (int) */
    lv_label_set_text_fmt(label3, "%d分%d秒", (int)(widget->Value_TimeLeft / 60), (int)(widget->Value_TimeLeft % 60));
    lv_obj_set_pos(label3, 170, 60);
    lv_obj_set_style_text_color(label3, lv_color_hex(FONT_RED_COLOR), LV_PART_MAIN);
    widget->Label_TimeLeft = label3;
    /*右上框*/
    lv_obj_t* Obj_rigth = Create_Obj(page_cont, 160, 90, 0xffffff, 0);
    lv_obj_set_pos(Obj_rigth, 280, 5);
    /*添加样式*/
    lv_obj_add_style(Obj_rigth, &style_shadow, LV_PART_MAIN);      // 底层：阴影
    lv_obj_add_style(Obj_rigth, &style_gradient, LV_PART_MAIN);    // 中层：渐变
    /*图标*/
    lv_obj_t* Img_rigth = Creat_Image(Obj_rigth, NEXTPRE_ICON);
    lv_obj_set_pos(Img_rigth, 10, 10);

    /*标题*/
    lv_obj_t* title2 = Creat_Label(Obj_rigth, "下节预览", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(title2, 35, 15);
    lv_obj_set_style_text_color(title2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* Obj2 = Create_Obj(Obj_rigth, 110, 3, FONT_GREEN_COLOR, 0);
    lv_obj_set_pos(Obj2, 30, 60);
    lv_obj_set_style_bg_color(Obj2, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);

    /*标题*/
    lv_obj_t* title6 = Creat_Label(page_cont, "腹部监控", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(title6, 390, 105);
    lv_obj_set_style_text_color(title6, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    /*完成标签*/
    lv_obj_t* label4 = Creat_Label(page_cont, "", basic_widget.Chinise_Font_Unit);
    lv_obj_align(label4, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_color(label4, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->Label_Finish = label4;

    // Therapy_Guide_Init(page_cont);      //初始化引导波形层
    //曲线表
    Creat_Chart(widget, page_cont);
    
}

/**
 * @brief 选择疗程界面（Flex 换行，自动填充全部疗程格）
 */
void Select_Widget2(lv_obj_t* page_cont, Select_Data_t* data)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, data->High_Obj, FONT_RED_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_set_pos(cont, 10, -8);

    /* 启用滚动 */
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);

    /* 关键：启用 Flex 布局 */
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);

    /* 横向排列，自动换行 */
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);

    /* 按钮之间的间隙 */
    lv_obj_set_flex_align(cont,
        LV_FLEX_ALIGN_START,   // 主轴
        LV_FLEX_ALIGN_START,   // 交叉轴
        LV_FLEX_ALIGN_CENTER   // 换行对齐
    );

    lv_obj_set_style_pad_row(cont, 3, 0);
    lv_obj_set_style_pad_column(cont, 25, 0);

    g_Treat_Point_cnt = 0;

    for (uint8_t s = 0; s < data->Stage_Num; s++) {
        for (uint8_t t = 1; t <= data->Sta_Table[s].total_times; t++) {
            Stage_Widget2(cont, data->Sta_Table[s].name, t, &data->sta[g_Treat_Point_cnt]);
            data->Treat_Point[g_Treat_Point_cnt++] = (Treat_Point_t){ s, t };
        }
    }
    data->Btn_MaxSum = g_Treat_Point_cnt;
    lv_obj_scroll_to_y(cont, 0, LV_ANIM_OFF);
}

/**
 * @brief 阶段疗程控件（单格：按钮 + 图标 + 标签）
 */
static void Stage_Widget2(lv_obj_t* page_cont,char *s, uint8_t t, Stage_Data_t* data)
{
    /*治疗按钮*/
    lv_obj_t* Btn = Create_Button(page_cont, 128, 92, FONT_WHITE_COLOR, 10, 1);
    lv_obj_t* Obj = Create_Obj(Btn, 128, 92, FONT_BLUE_COLOR, 0);
    lv_obj_align(Obj, LV_ALIGN_CENTER, 0, 0);
    data->obj = Obj;
    lv_obj_t* Img = Creat_Image(Btn, Stage_unSel_ICON);
    lv_obj_align(Img,LV_ALIGN_CENTER, 0, -8);
    lv_obj_set_style_image_recolor_opa(Img, 255, 0);
    lv_obj_set_style_image_recolor(Img, lv_color_hex(FONT_GRAY_COLOR), 0);
    data->btn = Btn;
    data->img = Img;
    /*按钮标签*/
    lv_obj_t* Label1 = Creat_Label(Btn, s, basic_widget.Chinise_Font_Btn);
    lv_obj_set_pos(Label1,0, 52);
    lv_obj_set_style_text_color(Label1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* Label2 = Creat_Label(Btn, "", basic_widget.Chinise_Font_Btn);
    lv_obj_set_pos(Label2, 45,52);
    lv_label_set_text_fmt(Label2, "·治疗%d", t);
    lv_obj_set_style_text_color(Label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
}

/**
 * @brief 疗程说明界面（标题 + 正文 + 图片 + 选择疗程按钮）
 */
void Instr_Widget(lv_obj_t* page_cont, Instr_Data_t* data)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_RED_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_set_pos(cont, -8, -8);

    /*说明框*/
    lv_obj_t* Obj = Create_Obj(cont, data->Obj_Wid, data->Obj_Hig, 0xffffff, 0);
    lv_obj_set_pos(Obj, 25, 5);
    /*添加样式*/
    lv_obj_add_style(Obj, &style_gradient, LV_PART_MAIN);    // 中层：渐变
    lv_obj_add_style(Obj, &style_shadow, LV_PART_MAIN);      // 底层：阴影
    /*标题*/
    lv_obj_t* title = Creat_Label(Obj, data->Title, basic_widget.Chinise_Font_Title);
    lv_obj_set_pos(title, 20, 20);
    lv_obj_set_style_text_color(title, lv_color_hex(data->Title_Color), LV_PART_MAIN);
    /*正文*/
    lv_obj_t* label = Creat_Label(Obj, "·", basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(label, 20, 55);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text = Creat_TextLabel(Obj, 200, 200, data->Text, basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(text, 30, 50);
    lv_obj_set_style_text_color(text, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_set_style_text_line_space(text, 10, 0);          // 行间距 10px

    /*图片*/
    lv_obj_t* Img = Creat_Image(cont, POS_ICON);
    lv_obj_set_pos(Img, 291, 19);

    /*选择疗程按钮*/
    lv_obj_t* btn = Create_Button(cont, 110, 40, data->Title_Color, 30, 0);
    lv_obj_set_pos(btn, 305, 200);
    lv_obj_t* label2 = Creat_Label(btn, "选择疗程", basic_widget.Chinise_Font_Btn);
    lv_obj_center(label2);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
    data->Btn = btn;
}
