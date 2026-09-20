/**
  ******************************************************************************
  * @文件名称   basic.c
  * @文件描述   UI 基础控件工厂 + 全局样式初始化 + 页面清理/跳转
  *            移植自 EDA_EMG_LVGL_PC\basic.c（同事 zqt 原装）
  * @适配说明  保持同事代码原样（Windows 路径由 ui_port/app_fs 'C' 盘翻译）；
  *            仅必要编译修正：
  *            - #include "lvgl/lvgl.h" → "lvgl.h"
  *            - lv_font_montserrat_26 → lv_font_montserrat_14（LVGL9.5 无 26）
  *            - Set_Chinese_Font / font 参数 → const lv_font_t*
  ******************************************************************************
  */

#include "basic.h"
#include "lang.h"
#include "menu_En_ui.h"   /* [我们] 英文主菜单（ui_page_en/） */
#include "app_therapy.h"  /* [我们] 安全：回主菜单全局兜底停治疗/采集（防漏停导致底层持续输出） */
#include "esp_log.h"

lv_style_t style_gradient;      //渐变样式 藕粉色
lv_style_t style_gradient2;     //渐变样式
lv_style_t style_gradient_gray;      //渐变样式 浅灰色
lv_style_t style_focused;       //聚焦时边框为0
lv_style_t style_shadow;        //阴影样式
lv_style_t style_BlueShadow;    //蓝色阴影样式
lv_style_t style_RedShadow;     //红色阴影样式
lv_style_t style_BlurShadow;    //模糊遮罩样式
lv_style_t style_RedKnob;       //红色把手样式
lv_style_t style_GaryKnob;      //灰色把手样式
lv_style_t style_BlueKnob;      //蓝色把手样式
lv_style_t style_YellowKnob;    //黄色把手样式

Basic_Widget_t basic_widget;
AppUI_t g_Ui = { 0 };

extern lv_my_ui_page_date_t  lv_my_ui_pageAss_t;
extern lv_my_ui_page_date_t  lv_my_ui_pageSet_t;
extern lv_my_ui_page_date_t  lv_my_ui_pageFrame_t;
extern lv_my_ui_page_date_t  lv_my_ui_pageMenu_t;
extern lv_my_ui_page_date_t  lv_my_ui_pageThe_t;
extern lv_my_ui_page_date_t  lv_my_ui_pagePosReh_t;

lv_my_ui_page_date_t* g_page_list[APP_SUM] = {  // 页面容量列表，用于页面跳转，需对应 PAGE_NAME 枚举
    &lv_my_ui_pageAss_t,
    &lv_my_ui_pageThe_t,
    &lv_my_ui_pageSet_t,
    &lv_my_ui_pageFrame_t,
    &lv_my_ui_pageMenu_t,
    &lv_my_ui_pagePosReh_t,
};

extern void Menu_ui(lv_obj_t* parent);

lv_my_ui_page_date_t* g_cur_page_date;  // 当前页面
Langs_Name_t Cur_Langs = LANGS_CH;      //初始化当前语言 中文


void chinese_font_init(void)
{
    /* 同事原装 Windows 路径：由 ui_port/app_fs 'C' 盘驱动翻译到 SPIFFS */
    lv_font_t* font_14 = lv_binfont_create("C:/Users/zqt/Desktop/PDJ_LVGL/png/font/lv_font_honorans_medium_14.bin");
    lv_font_t* font_btn = lv_binfont_create("C:/Users/zqt/Desktop/PDJ_LVGL/png/font/lv_font_Btn.bin");
    lv_font_t* font_title = lv_binfont_create("C:/Users/zqt/Desktop/PDJ_LVGL/png/font/lv_font_Title.bin");
    lv_font_t* font_text = lv_binfont_create("C:/Users/zqt/Desktop/PDJ_LVGL/png/font/lv_font_Text.bin");
    lv_font_t* font_unit = lv_binfont_create("C:/Users/zqt/Desktop/PDJ_LVGL/png/font/lv_font_Unit.bin");
    lv_font_t* font_30 = lv_binfont_create("C:/Users/zqt/Desktop/PDJ_LVGL/png/font/lv_font_30.bin");
    /* [我们] OTA 页专用子集字库（含联网/升级/失败/重试等新字，tools/gen_ota_font.js 生成） */
    lv_font_t* font_ota = lv_binfont_create("C:/Users/zqt/Desktop/PDJ_LVGL/png/font/lv_font_OTA.bin");
    /* [我们] 电极脱落弹窗专用子集字库（含脱/落/检/查/贴等 Btn 字库没有的字，tools/gen_elec_font.js 生成） */
    lv_font_t* font_elec = lv_binfont_create("C:/Users/zqt/Desktop/PDJ_LVGL/png/font/lv_font_Elec.bin");
    if ( font_14 == NULL ||
         font_btn == NULL ||
         font_title == NULL ||
         font_text == NULL ||
         font_unit == NULL ||
         font_30 == NULL)
    {
        ESP_LOGW("basic", "font init erro");
        return;
    }
    basic_widget.Chinese_Font_14 = font_14;
    basic_widget.Chinise_Font_Btn = font_btn;
    basic_widget.Chinise_Font_Title = font_title;
    basic_widget.Chinise_Font_Text = font_text;
    basic_widget.Chinise_Font_Unit = font_unit;
    basic_widget.Chinese_Font_30  = font_30;
    /* OTA 字库加载失败则回退到 Btn 字库（OTA 专用字会显示豆腐块，但不崩）。
     * 若看到 FAIL 日志，多半是 SPIFFS(storage) 没随固件全量烧录（app 快速烧录不含 storage）。 */
    if (font_ota == NULL) {
        ESP_LOGW("basic", "OTA font load FAIL (/spiffs/png/font/lv_font_OTA.bin) -> fallback Btn; "
                          "OTA-unique chars will be tofu. Need FULL flash incl. storage.");
    } else {
        ESP_LOGI("basic", "OTA font loaded OK (lv_font_OTA.bin)");
    }
    basic_widget.Chinise_Font_OTA = font_ota ? font_ota : font_btn;
    /* 电极脱落字库加载失败则回退 Btn（脱/落等新字会豆腐块，但不崩）；FAIL 多为 storage 未全量烧录。 */
    if (font_elec == NULL) {
        ESP_LOGW("basic", "Elec font load FAIL (/spiffs/png/font/lv_font_Elec.bin) -> fallback Btn; "
                          "elec-unique chars will be tofu. Need FULL flash incl. storage.");
    } else {
        ESP_LOGI("basic", "Elec font loaded OK (lv_font_Elec.bin)");
    }
    basic_widget.Chinise_Font_Elec = font_elec ? font_elec : font_btn;
}
/**
 * @author zqt
 * @brief 将字体设置成中文
 * @param obj
 */
void Set_Chinese_Font(lv_obj_t* obj, const lv_font_t* font)
{
    lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
}
/**
 * @author zqt
 * @brief 创建基本文本
 * @param page_cont
 * @param text
 * @return
 */
lv_obj_t* Creat_Label(lv_obj_t* page_cont, char* text, const lv_font_t* font)
{

    lv_obj_t* label = lv_label_create(page_cont);
    lv_label_set_text(label, text);
    Set_Chinese_Font(label, font);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_invalidate(label);

    return label;
}
/**
 * @author zqt
 * @brief 创建基本文本
 * @param page_cont
 * @param text
 * @return
 */
lv_obj_t* Creat_Label2(lv_obj_t* page_cont, uint16_t id, const lv_font_t* font)
{

    lv_obj_t* label = lv_label_create(page_cont);
    lv_label_set_text(label, lang_get_str(id));
    lv_obj_set_user_data(label, (void*)(uintptr_t)id);
    Set_Chinese_Font(label, font);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_invalidate(label);

    return label;
}

/**
 * @author zqt
 * @brief 创建基本文本框
 * @param page_cont
 * @param text
 * @return
 */
lv_obj_t* Creat_TextLabel(lv_obj_t* page_cont,int32_t w,int32_t h,char* text, const lv_font_t* font)
{

    lv_obj_t* Text = lv_textarea_create(page_cont);
    /*lv_obj_set_size(Text, w, h);*/

    lv_obj_set_width(Text, w);
    lv_obj_set_height(Text, LV_SIZE_CONTENT);

    lv_obj_set_style_text_color(Text, lv_color_hex(0x736f6f), LV_PART_MAIN);
    lv_textarea_add_text(Text,text);
    lv_obj_set_style_text_font(Text, basic_widget.Chinise_Font_Text, LV_PART_MAIN);
    Set_Chinese_Font(Text, font);
    lv_obj_set_scrollbar_mode(Text, LV_SCROLLBAR_MODE_OFF);          /*不显示滚动条*/
    lv_obj_set_style_text_color(Text, lv_color_hex(0x736f6f), LV_PART_MAIN);
    lv_obj_set_style_border_width(Text, LV_PART_MAIN, 0);
    lv_obj_set_style_bg_opa(Text, 0, LV_PART_MAIN);
    lv_textarea_set_max_length(Text, 255);

    lv_obj_invalidate(Text);
    return Text;
}


/**
 * @author zqt
 * @brief 创建基本背景
 * @param page_cont
 * @param w
 * @param h
 * @param color 背景颜色
 * @param flag 是否要显示背景颜色
 * @return
 */
lv_obj_t* Create_Obj(lv_obj_t* page_cont, int32_t w, int32_t h, uint32_t color, int flag)
{
    lv_obj_t* obj = lv_obj_create(page_cont);
    lv_obj_set_size(obj, w, h);
   /* lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);*/
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);          /*不显示滚动条*/
    lv_obj_set_style_border_opa(obj, 0, LV_PART_MAIN);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);                 /*禁用滑动*/
    if (flag) {
         lv_obj_set_style_bg_color(obj, lv_color_hex(color), LV_PART_MAIN);
    }
    return obj;
}
/**
 * @author zqt
 * @brief 创建基本背景
 * @param page_cont
 * @param w
 * @param h
 * @param color 背景颜色
 * @param flag 是否要显示背景颜色
 * @return
 */
lv_obj_t* Create_Obj2(lv_obj_t* page_cont, int32_t w, int32_t h, uint32_t color, int flag)
{
    lv_obj_t* obj = lv_obj_create(page_cont);
    //lv_obj_set_size(obj, w, h);
    lv_obj_set_width(obj, w);
    lv_obj_set_height(obj, LV_SIZE_CONTENT);
    /* lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);*/
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);          /*不显示滚动条*/
    lv_obj_set_style_border_opa(obj, 0, LV_PART_MAIN);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);                 /*禁用滑动条*/
    if (flag) {
        lv_obj_set_style_bg_color(obj, lv_color_hex(color), LV_PART_MAIN);
    }
    return obj;
}

/**
 * @brief 创建实线条
 * @param cont
 * @param w
 * @param h
 * @param point
 * @return
 */
lv_obj_t* creat_line(lv_obj_t* cont, int32_t w, int32_t h,
    const lv_point_precise_t* point, uint32_t point_num)
{
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 1);

    lv_obj_t* line = lv_line_create(cont);
    lv_obj_set_size(line, w, h);
    lv_line_set_points(line, point, point_num);
    lv_obj_add_style(line, &style_line, LV_PART_MAIN);
    return line;
}
/**
 * @brief 创建虚线条
 * @param cont
 * @param w
 * @param h
 * @param point
 * @param value 实段长
 * @return
 */
lv_obj_t* creat_DottedLine(lv_obj_t* cont, int32_t w, int32_t h,
    const lv_point_precise_t* point, uint32_t value)
{
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 1);

    lv_obj_t* line = lv_line_create(cont);
    lv_obj_set_size(line, w, h);
    lv_line_set_points(line, point, 2);
    lv_obj_add_style(line, &style_line, LV_PART_MAIN);
    lv_obj_set_style_line_width(line, 1, LV_PART_MAIN);
    lv_obj_set_style_line_dash_width(line, value, LV_PART_MAIN);   //实段长2px
    lv_obj_set_style_line_dash_gap(line, 2, LV_PART_MAIN);     //间隔4px
    lv_obj_set_style_line_rounded(line, false, LV_PART_MAIN);  // 必须关圆角，否则冲突
    lv_obj_set_style_line_color(line, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    return line;
}

/**
 * @author zqt
 * @brief 创建图像（原装：文件路径由 ui_port/app_fs 'C' 盘翻译+内存缓存）
 * @param page_cont
 * @param src           图像文件
 * @return
 */
lv_obj_t* Creat_Image(lv_obj_t* page_cont, void* src)
{
    lv_obj_t* image = lv_image_create(page_cont);
    if (src != NULL) {
        lv_image_set_src(image, src);
    }

    return image;
}
/**
 * @author zqt
 * @brief 创建基本滚轮部件
 * @param page_cont
 * @param w
 * @param h
 * @param option    选项
 * @param space     行高度
 * @return
 */
lv_obj_t* Create_Roller(lv_obj_t* page_cont, int32_t w, int32_t h, char* option, int32_t space)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_black());
    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_border_width(&style, 0);
    lv_style_set_radius(&style, 0);
    lv_style_set_text_line_space(&style, 36);
    static lv_style_t style2;
    lv_style_init(&style2);
    lv_style_set_bg_color(&style2, lv_color_hex(0x67B5BF));

    lv_obj_t* roller = lv_roller_create(page_cont);
    lv_obj_set_size(roller, w, h);
    lv_obj_add_style(roller, &style, 0);
    lv_obj_add_style(roller, &style2, LV_PART_SELECTED);

    lv_obj_set_style_bg_opa(roller, 100, LV_PART_MAIN);
    lv_roller_set_options(roller, option, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(roller, 1, LV_ANIM_OFF);
    lv_obj_set_style_text_font(roller, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_radius(roller, 10, LV_PART_MAIN);
    return roller;
}
/**
 * @author zqt
 * @brief 创建基本按钮
 * @param page_cont
 * @param w
 * @param h
 * @param value1    默认状态下的背景颜色
 * @param value3    设置弧度
 * @param flag      1:背景透明度为0，去除阴影
 * @return
 */
lv_obj_t* Create_Button(lv_obj_t* page_cont, int32_t w, int32_t h, uint32_t color,int32_t radVal,uint8_t flag)
{
    lv_obj_t* btn = lv_button_create(page_cont);
    lv_obj_set_size(btn, w, h);
   /* lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);   */                          /*添加状态：在单击对象时切换选中状态*/
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), LV_STATE_DEFAULT);          /*设置默认状态下的背景颜色*/
    //lv_obj_set_style_bg_color(btn, lv_color_hex(value2), LV_STATE_CHECKED);   /*设置单击状态下的背景颜色*/
    lv_obj_set_style_radius(btn, radVal, LV_PART_MAIN);
    lv_obj_add_style(btn, &style_focused, LV_STATE_FOCUSED);
    if (flag) {
        lv_obj_set_style_bg_opa(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);                    /* 去除按钮阴影 */
    }

    return btn;
}

/**
 * @author zqt
 * @brief 创建图片按钮
 * @param page_cont
 * @param w
 * @param h
 * @param scr    图片路径
 * @return
 */
lv_obj_t* Create_ImaButton(lv_obj_t* page_cont, int32_t w, int32_t h, void *scr)
{
    lv_obj_t* btn = lv_button_create(page_cont);
    lv_obj_set_size(btn, w, h);
    /* [我们] 移除 CHECKABLE：LVGL9 会触发 CHECKED 反色（见 0H 笔记 四）；同事原装有此 flag */
    /* lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);    */
    lv_obj_set_style_bg_opa(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);                    /* 去除按钮阴影 */
    lv_obj_set_style_radius(btn, 5, LV_PART_MAIN);
    lv_obj_t* btn_image = Creat_Image(btn, scr);
    lv_obj_center(btn_image);
    return btn;
}
/**
 * @author zqt
 * @brief 创建进度条
 * @param page_cont
 * @param color    部件背景颜色
 * @param value    设置最大值
 * @return
 */
lv_obj_t* Create_Bar(lv_obj_t* page_cont, uint32_t color, int32_t value)
{
    lv_obj_t* bar = lv_bar_create(page_cont);
    lv_bar_set_range(bar, 0, value);
    lv_obj_set_size(bar, 200, 20);
    lv_obj_set_style_bg_color(bar, lv_color_hex(color), LV_PART_INDICATOR);
    return bar;
}
/**
 * @author zqt
 * @brief 创建样式滑动条 背景透明度为0
 * @param page_cont
 * @param color    部件背景颜色
 * @param value    设置最大值
 * @return
 */
lv_obj_t* Create_Sliders(lv_obj_t* page_cont, uint32_t color, int32_t value)
{
    //初始化样式
    static  lv_style_t style_main, style_indicator, style_knob;
    // 主轨道样式 (透明度0 不显示)
    lv_style_init(&style_main);
    lv_style_set_pad_ver(&style_main, 6);                           // 上下内边距，确保轨道高度一致
    lv_style_set_bg_opa(&style_main, 0);
    // 进度条样式 (红色)
    lv_style_init(&style_indicator);
    lv_style_set_radius(&style_indicator, 10);                      // 同样胶囊形状

    // 创建滑块对象
    lv_obj_t* slider = lv_slider_create(page_cont);
    lv_obj_set_size(slider, 300, 15); // 宽度适配，高度要大于把手直径
    lv_obj_set_style_bg_color(slider, lv_color_hex(color), LV_PART_INDICATOR);
    // 应用样式
    lv_obj_add_style(slider, &style_main, LV_PART_MAIN);
    lv_obj_add_style(slider, &style_indicator, LV_PART_INDICATOR);
    lv_obj_add_style(slider, &style_GaryKnob, LV_PART_KNOB);
    //设置初始值和范围
    lv_slider_set_range(slider, 0, value);
    lv_slider_set_value(slider, 0, LV_ANIM_OFF);                   // 初始位置 30%

    return slider;
}
/**
 * @author zqt
 * @brief 初始化点击样式 效果渐变 右下角阴影
 * @param
 * @return
 */
void style_checked_init(void)
{
    /***创建藕粉色渐变样式 从上到下 渐变 上深下浅  跟右下角阴影组合*****/
    lv_style_init(&style_gradient);
    lv_style_set_bg_opa(&style_gradient, 60);
    lv_style_set_bg_color(&style_gradient, lv_color_hex(0xE6D6DA));//0xdccdd1
    lv_style_set_bg_grad_dir(&style_gradient, LV_GRAD_DIR_VER);
    lv_style_set_bg_grad_color(&style_gradient, lv_color_hex(0xFFF9FA));    //0xE6D6DA 0xF0E0E4 0xF9EEF0 0xFCF5F6 0xFFF9FA
    lv_style_set_bg_main_stop(&style_gradient, 0);
    lv_style_set_bg_grad_stop(&style_gradient, 80);
    lv_style_set_radius(&style_gradient, 15);
    lv_style_set_border_width(&style_gradient, 0);
    // 内边距
    // lv_style_set_pad_bottom(&style_gradient, 2);
    // lv_style_set_pad_right(&style_gradient, 0);
    // lv_style_set_pad_all(&style_gradient,0);
    // 裁剪圆角（重要！）
    lv_style_set_clip_corner(&style_gradient, true);

    /***创建浅灰色渐变样式 从上到下 渐变 *****/
    lv_style_init(&style_gradient_gray);
    lv_style_set_bg_opa(&style_gradient_gray, 200);
    lv_style_set_bg_color(&style_gradient_gray, lv_color_hex(0xD7D8DA));//
    lv_style_set_bg_grad_dir(&style_gradient_gray, LV_GRAD_DIR_VER);
    lv_style_set_bg_grad_color(&style_gradient_gray, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_main_stop(&style_gradient_gray, 0);
    lv_style_set_bg_grad_stop(&style_gradient_gray, 255);
   /* lv_style_set_radius(&style_gradient_gray, 15);*/
    lv_style_set_border_width(&style_gradient_gray, 0);
    // 裁剪圆角（重要！）
    lv_style_set_clip_corner(&style_gradient_gray, true);

    /***创建藕粉色渐变样式 从下到上 渐变 上浅下深***/
    lv_style_init(&style_gradient2);
    // 渐变背景设置
    lv_style_set_bg_opa(&style_gradient2, 60);
    lv_style_set_bg_color(&style_gradient2, lv_color_hex(0xffffff));//0xdccdd1
    lv_style_set_bg_grad_dir(&style_gradient2, LV_GRAD_DIR_VER);
    lv_style_set_bg_grad_color(&style_gradient2, lv_color_hex(0xdccdd1));    //0xE6D6DA 0xF0E0E4 0xF9EEF0 0xFCF5F6 0xFFF9FA
    lv_style_set_bg_main_stop(&style_gradient2, 80);
    lv_style_set_bg_grad_stop(&style_gradient2, 250);
    // 边框设置
    lv_style_set_border_width(&style_gradient2, 0);
    // 裁剪圆角（重要！）
    lv_style_set_clip_corner(&style_gradient2, true);


    /***创建阴影样式 右下角阴影 跟渐变组合***/
    lv_style_init(&style_shadow);
    // 阴影设置
    lv_style_set_shadow_width(&style_shadow, 40);
    lv_style_set_shadow_color(&style_shadow, lv_color_hex(0xd0c3c6));
    lv_style_set_shadow_opa(&style_shadow, 30);
    lv_style_set_shadow_spread(&style_shadow, 1);
    lv_style_set_shadow_offset_x(&style_shadow, 11);
    lv_style_set_shadow_offset_y(&style_shadow, 12);

    /*创建四周阴影样式  蓝色阴影*/
    lv_style_init(&style_BlueShadow);
    // 阴影设置
    lv_style_set_shadow_width(&style_BlueShadow, 12);
    lv_style_set_shadow_color(&style_BlueShadow, lv_color_hex(0x68D0C3));
    lv_style_set_shadow_opa(&style_BlueShadow, 100);
    lv_style_set_shadow_spread(&style_BlueShadow, 1);
    lv_style_set_shadow_offset_x(&style_BlueShadow, 0);
    lv_style_set_shadow_offset_y(&style_BlueShadow, 0);
    // 阴影层也需要圆角，否则会露出直角
    lv_style_set_radius(&style_BlueShadow, 50);

    /*创建四周阴影样式  红色阴影*/
    lv_style_init(&style_RedShadow);
    // 阴影设置
    lv_style_set_shadow_width(&style_RedShadow, 12);
    lv_style_set_shadow_color(&style_RedShadow, lv_color_hex(0xF26378));
    lv_style_set_shadow_opa(&style_RedShadow, 100);
    lv_style_set_shadow_spread(&style_RedShadow, 1);
    lv_style_set_shadow_offset_x(&style_RedShadow, 0);
    lv_style_set_shadow_offset_y(&style_RedShadow, 0);
    // 阴影层也需要圆角，否则会露出直角
    lv_style_set_radius(&style_RedShadow, 50);

    /*创建模糊遮罩阴影样式 顶层阴影*/
    lv_style_init(&style_BlurShadow);
    // 阴影设置
    lv_style_set_shadow_width(&style_BlurShadow, 20);
    lv_style_set_shadow_color(&style_BlurShadow, lv_color_hex(0xffffff));
    lv_style_set_shadow_opa(&style_BlurShadow, 255);
    lv_style_set_shadow_spread(&style_BlurShadow, 9);
    lv_style_set_shadow_offset_x(&style_BlurShadow, 0);
    lv_style_set_shadow_offset_y(&style_BlurShadow, -5);
    // 阴影层也需要圆角，否则会露出直角
    lv_style_set_radius(&style_BlurShadow, 50);//


    // 把手样式 (红色圆圈 + 白色边框)
    lv_style_init(&style_RedKnob);
    lv_style_set_bg_color(&style_RedKnob, lv_color_hex(FONT_RED_COLOR)); // 红色填充
    lv_style_set_bg_opa(&style_RedKnob, LV_OPA_COVER);
    lv_style_set_radius(&style_RedKnob, LV_RADIUS_CIRCLE);             // 正圆形
    lv_style_set_border_width(&style_RedKnob, 10);                     // 边框宽度
    lv_style_set_border_color(&style_RedKnob, lv_color_hex(0xffffff)); // 白色边框
    lv_style_set_pad_all(&style_RedKnob, 6);                           // 视觉大小（比实际大一点）
    // 阴影设置
    lv_style_set_shadow_width(&style_RedKnob, 12);
    lv_style_set_shadow_color(&style_RedKnob, lv_color_hex(0xF26378));
    lv_style_set_shadow_opa(&style_RedKnob, 100);
    lv_style_set_shadow_spread(&style_RedKnob, 1);
    lv_style_set_shadow_offset_x(&style_RedKnob, 0);
    lv_style_set_shadow_offset_y(&style_RedKnob, 0);
    // 阴影层也需要圆角，否则会露出直角
    lv_style_set_radius(&style_RedKnob, 50);

    // 把手样式 (灰色圆圈 + 白色边框)
    lv_style_init(&style_GaryKnob);
    lv_style_set_bg_color(&style_GaryKnob, lv_color_hex(FONT_GRAY_COLOR)); // 红色填充
    lv_style_set_bg_opa(&style_GaryKnob, LV_OPA_COVER);
    lv_style_set_radius(&style_GaryKnob, LV_RADIUS_CIRCLE);             // 正圆形
    lv_style_set_border_width(&style_GaryKnob, 10);                     // 边框宽度
    lv_style_set_border_color(&style_GaryKnob, lv_color_hex(0xffffff)); // 白色边框
    lv_style_set_pad_all(&style_GaryKnob, 6);                           // 视觉大小（比实际大一点）
    // 阴影设置
    lv_style_set_shadow_width(&style_GaryKnob, 12);
    lv_style_set_shadow_color(&style_GaryKnob, lv_color_hex(FONT_GRAY_COLOR));
    lv_style_set_shadow_opa(&style_GaryKnob, 100);
    lv_style_set_shadow_spread(&style_GaryKnob, 1);
    lv_style_set_shadow_offset_x(&style_GaryKnob, 0);
    lv_style_set_shadow_offset_y(&style_GaryKnob, 0);
    // 阴影层也需要圆角，否则会露出直角
    lv_style_set_radius(&style_GaryKnob, 50);

    // 把手样式 (蓝色圆圈 + 白色边框)
    lv_style_init(&style_BlueKnob);
    lv_style_set_bg_color(&style_BlueKnob, lv_color_hex(FONT_BLUE_COLOR)); // 红色填充
    lv_style_set_bg_opa(&style_BlueKnob, LV_OPA_COVER);
    lv_style_set_radius(&style_BlueKnob, LV_RADIUS_CIRCLE);             // 正圆形
    lv_style_set_border_width(&style_BlueKnob, 10);                     // 边框宽度
    lv_style_set_border_color(&style_BlueKnob, lv_color_hex(0xffffff)); // 白色边框
    lv_style_set_pad_all(&style_BlueKnob, 6);                           // 视觉大小（比实际大一点）
    // 阴影设置
    lv_style_set_shadow_width(&style_BlueKnob, 12);
    lv_style_set_shadow_color(&style_BlueKnob, lv_color_hex(FONT_GRAY_COLOR));
    lv_style_set_shadow_opa(&style_BlueKnob, 100);
    lv_style_set_shadow_spread(&style_BlueKnob, 1);
    lv_style_set_shadow_offset_x(&style_BlueKnob, 0);
    lv_style_set_shadow_offset_y(&style_BlueKnob, 0);
    // 阴影层也需要圆角，否则会露出直角
    lv_style_set_radius(&style_BlueKnob, 50);

    // 把手样式 (黄色圆圈 + 白色边框)
    lv_style_init(&style_YellowKnob);
    lv_style_set_bg_color(&style_YellowKnob, lv_color_hex(FONT_YELLOW_COLOR)); // 红色填充
    lv_style_set_bg_opa(&style_YellowKnob, LV_OPA_COVER);
    lv_style_set_radius(&style_YellowKnob, LV_RADIUS_CIRCLE);             // 正圆形
    lv_style_set_border_width(&style_YellowKnob, 10);                     // 边框宽度
    lv_style_set_border_color(&style_YellowKnob, lv_color_hex(0xffffff)); // 白色边框
    lv_style_set_pad_all(&style_YellowKnob, 6);                           // 视觉大小（比实际大一点）
    // 阴影设置
    lv_style_set_shadow_width(&style_YellowKnob, 12);
    lv_style_set_shadow_color(&style_YellowKnob, lv_color_hex(FONT_GRAY_COLOR));
    lv_style_set_shadow_opa(&style_YellowKnob, 100);
    lv_style_set_shadow_spread(&style_YellowKnob, 1);
    lv_style_set_shadow_offset_x(&style_YellowKnob, 0);
    lv_style_set_shadow_offset_y(&style_YellowKnob, 0);
    // 阴影层也需要圆角，否则会露出直角
    lv_style_set_radius(&style_YellowKnob, 50);

    lv_style_init(&style_focused);
    // lv_style_set_outline_width(&style_focused, 0);
    // lv_style_set_outline_color(&style_focused, lv_color_hex(FONT_YELLOW_COLOR));
    // lv_style_set_border_width(&style_focused, 0);
    // lv_style_set_shadow_width(&style_focused, 0);


}
/**
 * @brief 页面销毁 只清除容器
 * @param
 */
void Page_Clean(void)
{
    if (g_Ui.page_container) {
        lv_obj_clean(g_Ui.page_container);
    }

}
/**
 * @brief 返回菜单页（[我们] 按当前语言进中文/英文主菜单）
 * @param
 */
void Ui_EnterMenu(void)
{
    /* [我们] 安全兜底：任何路径回到主菜单，都强制停治疗波 + 停采集。
     * 底层 STM32 无通信超时/看门狗自保，一旦上层漏发停止令会持续输出；
     * 各页面返回函数若已规范停过，这里幂等早退（IDLE 且无 push 时不发任何总线帧）。 */
    therapy_emergency_stop();

    Page_Clean();
    if (g_app_lang == LANG_EN)
        Menu_En_ui(g_Ui.page_container);
    else
        Menu_ui(g_Ui.page_container);
}

void Goto_MenuPage(void)
{
    Ui_EnterMenu();
}
