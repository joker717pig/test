#ifndef __CHILD_DYSPAGE_H__
#define __CHILD_DYSPAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1


#include "lvgl.h"
#include "menu_ui.h" 

    /* 页面的数组保存位置 */
    typedef enum {
        ChiDysPage_Instr,      //第1页 说明页
        ChiDysPage_StaSel,     //第2页 疗程选择页
        ChiDysPage_TreIns,     //第3页 阶段治疗说明页
        ChiDysPage_ParSet,     //第4页 参数设置页
        ChiDysPage_Treat,      //第5页 治疗页（障碍格无曲线页）
        ChiDysPage_SUM     // 最大界面容量
    }ChiDysPage_ID_t; 

    extern Param_Data_t Dys_Param_Data1;

    void Child_DysPage_ui(void);
    void Child_DysPage_Load(ChiDysPage_ID_t page_id);

    /* [我们] B6-B 通用守则清理接口（坑 6.8 / B2-4 / B3-3 同族，参照治疗格/保养格） */
    void Child_DysPage_LeaveGroup(void);
    void Dys_BackToMenu(void);

#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
