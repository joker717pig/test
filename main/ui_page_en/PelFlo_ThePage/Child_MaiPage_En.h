#ifndef __CHILD_MAIPAGE_EN_H__
#define __CHILD_MAIPAGE_EN_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1

#include "lvgl.h"
#include "menu_ui.h"

    typedef enum {
        ChiMaiPage_Instr,
        ChiMaiPage_StaSel,
        ChiMaiPage_TreIns,
        ChiMaiPage_ParSet,
        ChiMaiPage_Treat,
        ChiMaiPage_Chart1,
        ChiMaiPage_Chart2,
        ChiMaiPage_Chart3,
        ChiMaiPage_Chart4,
        ChiMaiPage_SUM
    }ChiMaiPage_En_ID_t;
    
    extern Param_Data_t Mai_En_Param_Data1;

    void Child_MaiPage_En_ui(void);
    void Child_MaiPage_En_Load(ChiMaiPage_En_ID_t page_id);
    void Child_MaiPage_En_LeaveGroup(void);
    void Mai_En_BackToMenu(void);

#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*__CHILD_MAIPAGE_EN_H__*/
