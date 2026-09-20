#ifndef __CHILD_THEPAGE_EN_H__
#define __CHILD_THEPAGE_EN_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1

#include "lvgl.h"
#include "menu_ui.h"    /* [我们] Param_Data_t / Treat_Timer_Ctx_t（The_En_Param_Data1 extern 用） */

    /* 页面的数组保存位置（英文版独立枚举类型） */
    typedef enum {
        ChiThePage_Instr,      //第1页 说明页
        ChiThePage_StaSel,     //第2页 疗程选择页
        ChiThePage_TreIns,     //第3页 阶段治疗说明页
        ChiThePage_ParSet,     //第4页 参数设置页
        ChiThePage_Treat,      //第5页 治疗页
        ChiThePage_Chart1,     //第6页 曲线图1
        ChiThePage_Chart2,     //第7页 曲线图2
        ChiThePage_Chart3,     //第8页 曲线图3
        ChiThePage_Chart4,     //第9页 曲线图4（PC 预留未实现，勿用）
        ChiThePage_SUM         // 最大界面容量
    }ChiThePage_En_ID_t;

    void Child_ThePage_En_ui(void);
    void Child_ThePage_En_Load(ChiThePage_En_ID_t page_id);
    void The_En_BackToMenu(void);

    /* [我们] B5-C 通用守则清理接口（坑 6.8 / B2-4 / B3-3 同族） */
    void Child_ThePage_En_LeaveGroup(void);

    /* [我们] 阶段C 治疗控制：Page4 参数对象（强度1/2 存于此），供英文事件文件/引擎读取 */
    extern Param_Data_t The_En_Param_Data1;

#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*__CHILD_THEPAGE_EN_H__*/
