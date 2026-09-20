#ifndef __CHILD_KGEPAGE_H__
#define __CHILD_KGEPAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1


#include "lvgl.h"
#include "menu_ui.h" 

    /* 页面的数组保存位置 */
    typedef enum {
        ChiKgePage_Instr,      //第1页 说明页
        ChiKgePage_StaSel,     //第2页 疗程选择页
        ChiKgePage_TreIns,     //第3页 阶段治疗说明页
        ChiKgePage_ParSet,     //第4页 参数设置页
        ChiKgePage_Treat,      //第5页 治疗页
        ChiKgePage_Chart1,     //第6页 曲线图1
        ChiKgePage_Chart2,     //第7页 曲线图2
        ChiKgePage_Chart3,     //第8页 曲线图3
        ChiKgePage_Chart4,     //第9页 曲线图4（PC 预留未实现，勿用）
        ChiKgePage_SUM     // 最大界面容量
    }ChiKgePage_ID_t;

    extern Param_Data_t Kge_Param_Data1;

    void Child_KgePage_ui(void);
    void Child_KgePage_Load(ChiKgePage_ID_t page_id);

    /* [我们] B6-C 通用守则清理接口（坑 6.8 / B2-4 / B3-3 同族，参照治疗格/保养格） */
    void Child_KgePage_LeaveGroup(void);
    void Kge_BackToMenu(void);

#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
