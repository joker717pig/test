#ifndef __CHILD_MAIPAGE_H__
#define __CHILD_MAIPAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1


#include "lvgl.h"
#include "menu_ui.h" 

    /* 页面的数组保存位置 */
    typedef enum {
        ChiMaiPage_Instr,      //第1页 说明页
        ChiMaiPage_StaSel,     //第2页 疗程选择页
        ChiMaiPage_TreIns,     //第3页 阶段治疗说明页
        ChiMaiPage_ParSet,     //第4页 参数设置页
        ChiMaiPage_Treat,      //第5页 治疗页
        ChiMaiPage_Chart1,     //第6页 曲线图1
        ChiMaiPage_Chart2,     //第7页 曲线图2
        ChiMaiPage_Chart3,     //第8页 曲线图3
        ChiMaiPage_Chart4,     //第9页 曲线图4（PC 预留未实现，勿用）
        ChiMaiPage_SUM          // 最大界面容量
    }ChiMaiPage_ID_t;

   extern Param_Data_t Mai_Param_Data1;

    void Child_MaiPage_ui(void);
    void Child_MaiPage_Load(ChiMaiPage_ID_t page_id);

    /* [我们] B6-A 通用守则清理接口（坑 6.8 / B2-4 / B3-3 同族，参照治疗格） */
    void Child_MaiPage_LeaveGroup(void);
    void Mai_BackToMenu(void);

#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
