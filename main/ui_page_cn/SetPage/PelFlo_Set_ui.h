#ifndef __PELFLO_SET_UI_H__
#define __PELFLO_SET_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#define     KNOB_UNSEL_ICON      "C:/Users/zqt/Desktop/PDJ_LVGL/png/SetPage/knob_unSel.bin"         //未选中图标
#define     KNOB_SEL_ICON        "C:/Users/zqt/Desktop/PDJ_LVGL/png/SetPage/knob_Sel.bin"           //选中图标
#define     SETFINISH_ICON       "C:/Users/zqt/Desktop/PDJ_LVGL/png/SetPage/set_finish.bin"         //设置完成图标
    // 页面的数组保存位置
    typedef enum {
        Btn_Volume,       //音量调节按钮
        Btn_Bright,      //亮度调节按钮
        Btn_Language,    //语言按钮
        Btn_Date,        //日期时间设置按钮
        Btn_OTA,        //OTA设置按钮
        Btn_SUM         
    }Btn_Name;

    // 页面的数组保存位置
    typedef enum {
        Page_Volume,       //音量调节按钮
        Page_Bright,      //亮度调节按钮
        Page_Language,    //语言按钮
        Page_Date,        //日期设置按钮
        Page_Time,        //时间设置按钮
        Page_OTA,         //OTA按钮
        Page_Menu,
        Page_Finish,
        Page_SUM
    }Set_PageID_t;
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
    }OTA_Widget_t;

    typedef struct
    {
        char*     Hour_value;
        char*     Minu_value;
        void*     Knob_ChiSrc;         //中文按钮点击图标
        void*     Knob_EngSrc;         //英文按钮点击图标
        int32_t   Value_Volume;        //音量值
        int32_t   Value_Bright;        //亮度值
        lv_obj_t* Btn[Btn_SUM];        //列表按钮
        lv_obj_t* Btn_China;           //中文按钮
        lv_obj_t* Btn_English;         //英文按钮
        lv_obj_t* Btn_Page3Next;       //第四页的下一步按钮
        lv_obj_t* Btn_Page4Next;       //第五页的下一步按钮
        lv_obj_t* Roller_Year;         //年份滚轮
        lv_obj_t* Roller_Month;        //月份滚轮
        lv_obj_t* Roller_Day;          //日滚轮 
        lv_obj_t* Roller_Hour;         //小时滚轮
        lv_obj_t* Roller_Min;          //分钟滚轮
        lv_obj_t* Btn_Sure;            //确定按钮
        lv_timer_t* Timer;             //定时器
        lv_obj_t* Slider_Volume;       //音量滑动条
        lv_obj_t* Slider_Bright;       //亮度滑动条
        lv_obj_t* Btn_VolBack;         //音量页返回按钮
        lv_obj_t* Btn_BriBack;         //亮度页返回按钮
        lv_obj_t* Btn_DateBack;        //日期页返回按钮
        lv_obj_t* Label_Sure;          //确定按钮标签
        lv_obj_t* Label_DatBack;       //日期设置页返回按钮标签
        lv_obj_t* Label_VolBack;       //音量设置页返回按钮标签
        lv_obj_t* Label_BriBack;       //亮度设置页返回按钮标签
        lv_obj_t* Label_P3Next;        //第三页的下一步按钮标签
        lv_obj_t* Label_P4Next;        //第四页的下一步按钮标签
        lv_obj_t* Label_VolValue;      // [我们] 音量值实时显示标签（右端数字）
        lv_obj_t* Label_BriValue;      // [我们] 亮度值实时显示标签（右端数字）
    }Set_Widget_t;

    // 页面的数组保存位置
    typedef enum {
        SetPage0,       //菜单列表
        SetPage1,       //音量调节
        SetPage2,       //亮度调节
        SetPage3,       //语言选择页
        SetPage4,       //日期设置页
        SetPage5,       //时间设置页
        SetPage6,       //设置完成页
        SetPage_SUM     // 最大界面容量
    }SetPage_Name_t;    /* [我们] static enum → typedef（头文件 static enum 每编译单元复制 → 链接冲突） */

    void PelFlo_Set_ui(void);
    void Set_Page_Load(Set_PageID_t page_id);

    /* [我们] 设置页资源清理（切页/返回前调用，防悬垂崩溃，坑 6.8 / B2-4 同族） */
    void Set_StopTimer(void);
    void Set_LeaveGroup(void);
    void Set_BackToMenu(void);
    void Set_BackToList(void);   /* [我们] 返回设置列表（二级菜单） */
    void SetPage_SetBar_Value(lv_obj_t * bar,int32_t v);
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
