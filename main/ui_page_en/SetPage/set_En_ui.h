/**
  ******************************************************************************
  * @文件名称   set_En_ui.h
  * @文件描述   英文设置页（两套 UI，阶段 E2；复制中文 SetPage 直译英文草稿）
  *            布局沿用中文 SetPage，仅文字英文化；符号统一 *_En 前缀防链接冲突。
  *            入口：英文主菜单 Setting → Set_En_ui()；返回经 Goto_MenuPage()
  *            （Ui_EnterMenu 按语言回对应菜单）。
  ******************************************************************************
  */

#ifndef __SET_EN_UI_H__
#define __SET_EN_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "basic.h"

#define     EN_KNOB_UNSEL_ICON     "C:/Users/zqt/Desktop/PDJ_LVGL/png/SetPage/knob_unSel.bin"    //未选中图标
#define     EN_KNOB_SEL_ICON       "C:/Users/zqt/Desktop/PDJ_LVGL/png/SetPage/knob_Sel.bin"      //选中图标
#define     EN_SETFINISH_ICON      "C:/Users/zqt/Desktop/PDJ_LVGL/png/SetPage/set_finish.bin"    //设置完成图标

    /* 英文设置页按钮 ID（与中文 Btn_Name 一致） */
    typedef enum {
        En_Btn_Volume,       //音量调节按钮
        En_Btn_Bright,       //亮度调节按钮
        En_Btn_Language,     //语言按钮
        En_Btn_Date,         //日期时间设置按钮
        En_Btn_OTA,
        En_Btn_SUM
    } En_Btn_Name;

    /* 英文设置页子页 ID（与中文 Set_PageID_t 一致，保证导航/返回逻辑一致） */
    typedef enum {
        En_Page_Volume,      //音量调节
        En_Page_Bright,      //亮度调节
        En_Page_Language,    //语言
        En_Page_Date,        //日期设置
        En_Page_Time,        //时间设置
        En_Page_OTA,
        En_Page_Menu,        //设置列表
        En_Page_Finish,      //设置完成
        En_Page_SUM
    } Set_En_PageID_t;

       typedef struct
    {
        lv_obj_t* Btn_Sure;        //确认按钮（是/开始升级/重试/检查中…）
        lv_obj_t* Btn_Back;        //返回按钮（升级完成后变"点击重启"）
        lv_obj_t* Label_OTA;       //OTA主提示标签
        lv_obj_t* Label_Net;       //[我们] 联网状态标签（联网中/成功/失败）
        lv_obj_t* Bar_STM32;       //STM32进度条
        lv_obj_t* Bar_ESP32;       //ESP32进度条
        lv_obj_t* Bar_Bin;         //图片进度条
        lv_obj_t* Label_STM32;     //STM32进度标签（条内百分比）
        lv_obj_t* Label_ESP32;     //ESP32进度标签
        lv_obj_t* Label_Bin;       //图片进度标签
        lv_obj_t* Label_Ver_STM32; //[我们] STM32 版本号标签（当前→云端）
        lv_obj_t* Label_Ver_Bin;   //[我们] 图片版本号标签
        lv_obj_t* Label_Ver_ESP32; //[我们] ESP32 版本号标签
        lv_obj_t* Label_Sure;      //确认按钮标签
        lv_obj_t* Label_Back;      //返回按钮标签
        lv_timer_t* Timer;         //轮询定时器（~150ms 刷 g_ota_ui 状态）
    }OTA_Widget_EN_t;

    /* 英文设置页控件结构体（独立于中文 Set_Widget_t；字段名沿用） */
    typedef struct {
        char*     Hour_value;
        char*     Minu_value;
        void*     Knob_ChiSrc;         //中文按钮点击图标
        void*     Knob_EngSrc;         //英文按钮点击图标
        int32_t   Value_Volume;        //音量值
        int32_t   Value_Bright;        //亮度值
        lv_obj_t* Btn[En_Btn_SUM];     //列表按钮
        lv_obj_t* Btn_China;           //中文按钮
        lv_obj_t* Btn_English;         //英文按钮
        lv_obj_t* Btn_Page3Next;       //语言页下一步
        lv_obj_t* Btn_Page4Next;       //日期页下一步
        lv_obj_t* Roller_Year;         //年份滚轮
        lv_obj_t* Roller_Month;        //月份滚轮
        lv_obj_t* Roller_Day;          //日滚轮
        lv_obj_t* Roller_Hour;         //小时滚轮
        lv_obj_t* Roller_Min;          //分钟滚轮
        lv_obj_t* Btn_Sure;            //确定按钮
        lv_timer_t* Timer;             //定时器
        lv_obj_t* Slider_Volume;       //音量滑动条
        lv_obj_t* Slider_Bright;       //亮度滑动条
        lv_obj_t* Btn_VolBack;         //音量页返回
        lv_obj_t* Btn_BriBack;         //亮度页返回
        lv_obj_t* Btn_DateBack;        //日期页返回
        lv_obj_t* Label_Sure;          //确定按钮标签
        lv_obj_t* Label_DatBack;       //日期返回按钮标签
        lv_obj_t* Label_VolBack;       //音量返回按钮标签
        lv_obj_t* Label_BriBack;       //亮度返回按钮标签
        lv_obj_t* Label_P3Next;        //语言页下一步标签
        lv_obj_t* Label_P4Next;        //日期页下一步标签
        lv_obj_t* Label_VolValue;      //音量值实时显示标签
        lv_obj_t* Label_BriValue;      //亮度值实时显示标签
    } Set_En_Widget_t;

    /* 英文设置页界面下标（与中文 SetPage_Name_t 一致） */
    typedef enum {
        En_SetPage0,       //菜单列表
        En_SetPage1,       //音量调节
        En_SetPage2,       //亮度调节
        En_SetPage3,       //语言选择
        En_SetPage4,       //日期设置
        En_SetPage5,       //时间设置
        En_SetPage6,       //设置完成
        En_SetPage_SUM
    } Set_En_Page_Name_t;

    void Set_En_ui(void);
    void Set_En_Page_Load(Set_En_PageID_t page_id);
    void OTA_En_Page_Enter(OTA_Widget_EN_t *w);

    /* [我们] 英文设置页资源清理（切页/返回前调用，防悬垂崩溃，坑 6.8 / B2-4 同族） */
    void Set_En_StopTimer(void);
    void Set_En_LeaveGroup(void);
    void Set_En_BackToMenu(void);
    void Set_En_BackToList(void);   /* [我们] 返回设置列表（二级菜单） */

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __SET_EN_UI_H__ */
