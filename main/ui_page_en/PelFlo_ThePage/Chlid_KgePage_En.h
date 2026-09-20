#ifndef __CHILD_KGEPAGE_EN_H__
#define __CHILD_KGEPAGE_EN_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1

#include "lvgl.h"

    typedef enum {
        ChiKgePage_Instr,
        ChiKgePage_StaSel,
        ChiKgePage_TreIns,
        ChiKgePage_ParSet,
        ChiKgePage_Treat,
        ChiKgePage_Chart1,
        ChiKgePage_Chart2,
        ChiKgePage_Chart3,
        ChiKgePage_Chart4,
        ChiKgePage_SUM
    }ChiKgePage_En_ID_t;

    void Child_KgePage_En_ui(void);
    void Child_KgePage_En_Load(ChiKgePage_En_ID_t page_id);
    void Child_KgePage_En_LeaveGroup(void);
    void Kge_En_BackToMenu(void);

#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*__CHILD_KGEPAGE_EN_H__*/
