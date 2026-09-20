#ifndef __CHILD_DYSPAGE_EN_H__
#define __CHILD_DYSPAGE_EN_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1

#include "lvgl.h"
#include "menu_ui.h"

    typedef enum {
        ChiDysPage_Instr,
        ChiDysPage_StaSel,
        ChiDysPage_TreIns,
        ChiDysPage_ParSet,
        ChiDysPage_Treat,      //治疗页（障碍格无曲线页）
        ChiDysPage_SUM
    }ChiDysPage_En_ID_t;

    extern Param_Data_t Dys_En_Param_Data1;

    void Child_DysPage_En_ui(void);
    void Child_DysPage_En_Load(ChiDysPage_En_ID_t page_id);
    void Child_DysPage_En_LeaveGroup(void);
    void Dys_En_BackToMenu(void);

#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*__CHILD_DYSPAGE_EN_H__*/
