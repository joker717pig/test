#ifndef __MAIN_UI_H__
#define __MAIN_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "basic.h"
#include "menu_ui.h"

#define ECG_SAMPLE_COUNT 1000

    static const lv_coord_t ecg_sample[ECG_SAMPLE_COUNT] = {
     55, 56, 57, 58, 59, 60, 60, 61, 62, 63,
    };
    static const lv_coord_t ecg_sample2[ECG_SAMPLE_COUNT] = {
     55, 56, 57, 58, 59, 60, 60, 61, 62, 63,
    };

#define CHART_N  500
#define CHART_Y_MIN  0
#define CHART_Y_MAX  100
extern uint32_t g_write_idx;
extern lv_chart_cursor_t* cur;

extern lv_coord_t HisBtn_origin_y;
extern lv_coord_t AssBtn_origin_y;
extern lv_coord_t Obj_origin_y;
extern lv_chart_series_t* ser_green;
extern lv_chart_series_t* ser_red;

#define MAXITEM 30

typedef struct {
    uint8_t Menu_Total_Item;               // 数据条目
    uint64_t CheckItem;             // 选中准备删除或保存的条目
    lv_obj_t* Menu[MAXITEM + 1];      // 最多保存30条数据
}Record_Data_t;

typedef struct
{
    lv_obj_t* History_Btn;              //查看历史按钮
    lv_obj_t* Ass_Btn;                  //开始评估按钮
    lv_obj_t* History_Btn_Label;        //查看历史按钮文本
    lv_obj_t* Ass_Btn_Label;            //开始评估按钮文本
    lv_obj_t* Max_Label;                //最大肌电位
    lv_obj_t* Instan_Label;             //瞬时肌电位
    lv_obj_t* RemTime_Label;            //剩余时间
    lv_obj_t* Phase_Label;              /* [我们] 当前评估阶段名（前静息期/快速收缩期/...） */
    lv_obj_t* Action_Label;             /* [我们] 当前动作名（放松/用力/保持用力） */
    lv_obj_t* Next_Action_Label;        /* [我们] 下节动作名（下节预览） */
    lv_obj_t* Chart;                    //曲线表
    lv_timer_t* Report_Timer;           //生成报告定时器
    lv_timer_t* Chart_Timer;            //曲线定时器

    lv_obj_t* Datamenu;                 // 数据菜单部件
    Record_Data_t RecordData;         //记录的数据，主要是菜单部件
        lv_obj_t* Close_Button;             //关闭界面按钮
    lv_obj_t* Record_Save_Button;       //记录保存按钮
    lv_obj_t* Record_Delete_Button;     //记录删除按钮
    Scroll_Data_t* Scroll;              //页面滚动数据
    Window_Data_t Window;               // 返回确认弹窗控件
}Ass_Widget_t;


/* 页面ID */
typedef enum {
    AssPage1 = 0,   //盆底评估第1页 说明页
    AssPage2,       //盆底评估第2页 评估中
    AssPage3,       //盆底评估第3页 评估完成
    AssPage4,       //盆底评估第4页 检测报告
    AssPage5,       //盆底评估第5页 健康档案
    AssPage_MAX     // 最大界面容量
}Ass_PageID_t;

//extern lv_my_ui_page_date_t* g_ChildPage_list[];

extern Ass_Widget_t Ass_Widget;
extern lv_my_child_page_date_t lv_my_AssPage1_t;
extern lv_my_child_page_date_t lv_my_AssPage2_t;
extern lv_my_child_page_date_t lv_my_AssPage3_t;
extern lv_my_child_page_date_t lv_my_AssPage4_t;
extern lv_my_child_page_date_t lv_my_AssPage5_t;

void PelFlo_Ass_ui(void);
//void PelFloAss_Page4_widget(Ass_Widget_t* widget, lv_obj_t* page_cont);
void Record_wdiget(Ass_Widget_t* widget, lv_obj_t* page_cont);

void Ass_Page_Load(Ass_PageID_t page_id);

/* [我们] B2 评估页资源清理/按键接口（切页/返回前调用，防悬垂崩溃，坑 6.8 同族） */
void Ass_Stop_Timers(void);
void Ass_LeaveGroup(void);
void Ass_BackToMenu(void);
void Ass_ShowBackConfirm(void); // 显示返回确认弹窗

/* [我们] 评估阶段引导波形（示意底图）：按阶段画"用力(高)/静息(低)"阶梯横线，阶段切换时重画 */
void Ass_Guide_Update(uint8_t phase_idx);
void Record_EcgTable_wdiget(Ass_Widget_t* widget, lv_obj_t* page_cont);
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*MY_UI_PAGE1_H*/