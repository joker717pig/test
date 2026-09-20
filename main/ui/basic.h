/**
  ******************************************************************************
  * @文件名称   basic.h
  * @文件描述   UI 基础控件工厂 + 全局样式 + 页面管理框架
  *            移植自 EDA_EMG_LVGL_PC\basic.h（同事 zqt，LVGL v9 480×320 横屏）
  * @适配说明  保持同事代码结构；仅必要编译修正（路径原样由 ui_port/app_fs 翻译）：
  *            - #include "lvgl/lvgl.h" → "lvgl.h"（ESP-IDF 组件规范）
  *            - static enum PAGE_NAME → typedef（头文件 static 会每编译单元复制）
  *            - font 参数类型 lv_obj_t* → const lv_font_t*（LVGL9 严格类型）
  ******************************************************************************
  */

#ifndef __BASIC_H__
#define __BASIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#define FONT_WHITE_COLOR                0xffffff                //白色字体
#define FONT_GRAY_COLOR                 0x736f6f                //灰色字体
#define FONT_GREEN_COLOR                0x68D0C3               //绿色字体
#define FONT_BLACK_COLOR                0x000000                //黑色字体
#define FONT_RED_COLOR                  0xF26378                //红色字体
#define FONT_BLUE_COLOR                 0x6786f1                //蓝色字体
#define FONT_YELLOW_COLOR               0xe8b261                //黄色色字体

#define MAIN_BG_COLOR                   0x736f6f
#define MAIN_HEAD_PART_WIDE             480                    /*主屏幕_头部_宽度*/
#define MAIN_HEAD_PART_HIGE             30                      /*主屏幕_头部_高度*/

#define TIME_BG_COLOR                   0x000521              /*主屏幕背景颜色0X1011130x1516180x080515医疗蓝0x243546*/
    typedef struct {
        lv_font_t* Chinise_Font_Text;           /*说明框正文字体*/
        lv_font_t* Chinise_Font_Title;          /*说明框标题字体*/
        lv_font_t* Chinise_Font_Btn;            /*按钮字体*/
        lv_font_t* Chinese_Font_14;             /*字体12号*/
        lv_font_t* Chinese_Font_30;             /*字体30号*/
        lv_font_t* Chinise_Font_Unit;           /*参数字体字号*/
        lv_font_t* Chinise_Font_OTA;            /*[我们] OTA升级页专用子集字库(含联网/升级/失败等新字)*/
        lv_font_t* Chinise_Font_Elec;           /*[我们] 电极脱落弹窗专用子集字库(含脱/落/检/查/贴等新字, size15)*/
        lv_obj_t*  Record_BackBtn;              /*档案页的返回菜单页按钮*/
        lv_obj_t*  Report_BackBtn;              /*检测报告页的返回菜单页按钮*/
    }Basic_Widget_t;

    extern Basic_Widget_t basic_widget;

    // 页面的数组保存位置
    typedef enum {
        PelFlo_AssPage,     //盆底评估页
        PelFlo_ThePage,     //盆底治疗页
        Set_Page,           // 日历界面
        Fram_Page,          //
        Menu_Page,          //菜单页面
        PosReh_Page,        //菜单页面
        APP_SUM             // 最大界面容量
    } PAGE_NAME_t;

    typedef enum {
        LANGS_CH,       //中文
        LANGS_EN,       //英文
        LANHS_SUM       //语言数量
    }Langs_Name_t;

    extern Langs_Name_t Cur_Langs;

    typedef struct
    {
        char* Langs[LANHS_SUM];
    }Langs_t;


    /* 全局UI结构 */
    typedef struct {
        lv_obj_t* top_bar;          // 顶部状态栏
        lv_obj_t* page_container;   // 页面容器
    } AppUI_t;

    extern AppUI_t g_Ui;

    extern lv_style_t style_gradient2;
    extern lv_style_t style_gradient;
    extern lv_style_t style_gradient_gray;
    extern lv_style_t style_shadow;
    extern lv_style_t style_BlueShadow;
    extern lv_style_t style_RedShadow;
    extern lv_style_t style_BlurShadow;
    extern lv_style_t style_RedKnob;       //红色把手样式
    extern lv_style_t style_GaryKnob;      //灰色把手样式
    extern lv_style_t style_BlueKnob;      //灰色把手样式
    extern lv_style_t style_YellowKnob;    //灰色把手样式
    extern lv_style_t style_focused;       //聚焦时边框为0
    /**
     * 管理主页面 盆底评估 盆底治疗 产后修复 设置
     * @brief 打开页面:page_open
     * @brief 关闭页面:page_close
     */
    typedef struct {
        lv_obj_t* page_cont;        // 页面容器
        uint16_t(*open)(void);      // 打开页面，->错误/警告
        uint16_t(*close)(void);
        uint16_t(*jump)(lv_obj_t* page);    // 实现跳转到page页面
    }lv_my_ui_page_date_t;

    /**
      * 管理主页面下的子页面
      * @brief 打开页面:page_open
      * @brief 关闭页面:page_close
    */
    typedef struct {
        lv_obj_t* page_cont;        // 页面容器
        uint16_t(*jump)(lv_obj_t* page1,lv_obj_t* page2);    // 实现跳转到page页面
    }lv_my_child_page_date_t;


   void chinese_font_init(void);
    void Set_Chinese_Font(lv_obj_t* obj, const lv_font_t* font);
    lv_obj_t* Creat_Label(lv_obj_t* page_cont, char* text, const lv_font_t* font);
    lv_obj_t* Create_Obj(lv_obj_t* page_cont, int32_t w, int32_t h, uint32_t color, int flag);
    lv_obj_t* creat_line(lv_obj_t* cont, int32_t w, int32_t h,
    const lv_point_precise_t* point, uint32_t point_num);
    lv_obj_t* Creat_Image(lv_obj_t* page_cont, void* src);
    lv_obj_t* Create_Roller(lv_obj_t* page_cont, int32_t w, int32_t h, char* option, int32_t space);
    lv_obj_t* Create_Button(lv_obj_t* page_cont, int32_t w, int32_t h, uint32_t color, int32_t radVal, uint8_t flag);
    lv_obj_t* Create_Sliders(lv_obj_t* page_cont, uint32_t color, int32_t value);
    lv_obj_t* Create_ImaButton(lv_obj_t* page_cont, int32_t w, int32_t h, void* scr);
    lv_obj_t* Creat_TextLabel(lv_obj_t* page_cont, int32_t w, int32_t h, char* text, const lv_font_t* font);
    lv_obj_t* creat_DottedLine(lv_obj_t* cont, int32_t w, int32_t h,
    const lv_point_precise_t* point, uint32_t value);
    void style_checked_init(void);
    lv_obj_t* Creat_Label2(lv_obj_t* page_cont, uint16_t id, const lv_font_t* font);
    lv_obj_t* Create_Obj2(lv_obj_t* page_cont, int32_t w, int32_t h, uint32_t color, int flag);
    lv_obj_t* Create_Bar(lv_obj_t* page_cont, uint32_t color, int32_t value);    
    void Page_Clean(void);
    void Goto_MenuPage(void);
    void Ui_EnterMenu(void);   /* [我们] 两套 UI：按 g_app_lang 进中文/英文主菜单 */
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
