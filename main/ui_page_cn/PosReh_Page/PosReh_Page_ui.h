#ifndef __POSREH_PAGE_UI_H__
#define __POSREH_PAGE_UI_H__
#ifdef __cplusplus
extern "C" {
#endif


#include "lvgl.h"
#include "menu_ui.h"

#define     WOMEN_ICON          "C:/Users/zqt/Desktop/PDJ_LVGL/png/PosRehPage/Women.bin"             //女人图标
#define     OBJ_UNSEL_ICON      "C:/Users/zqt/Desktop/PDJ_LVGL/png/PosRehPage/Obj_unSel.bin"         //说明框未选中图标
#define     OBJ_SEL_ICON        "C:/Users/zqt/Desktop/PDJ_LVGL/png/PosRehPage/Obj_Sel.bin"           //说明框选中图标
#define     PLUSE_YELLOW_ICON   "C:/Users/zqt/Desktop/PDJ_LVGL/png/PosRehPage/pluse_yellow.bin"      //黄色脉冲图标
#define     SET_YELLOW_ICON     "C:/Users/zqt/Desktop/PDJ_LVGL/png/PosRehPage/set_yellow.bin"        //黄色设置图标
#define     PLU_YELLOW_ICON     "C:/Users/zqt/Desktop/PDJ_LVGL/png/PosRehPage/plu_yellow.bin"        //黄色加号图标
#define     MIN_YELLOW_ICON     "C:/Users/zqt/Desktop/PDJ_LVGL/png/PosRehPage/min_yellow.bin"        //黄色加号图标

    // 页面的数组保存位置
    typedef enum {
        PosRehPage1,       //盆底评估第1页 说明页
        PosRehPage2,       //盆底评估第2页 评估中
        PosRehPage3,       //盆底评估第3页 评估完成
        PosRehPage4,       //盆底评估第4页 检测报告
        PosRehPage5,       //盆底评估第5页 健康档案
        PosRehPage6,       //盆底评估第5页 健康档案
        PosRehPage_SUM         // 最大界面容量
    }PosReh_PageID_t;

   typedef enum {
        DiaRecti,       //腹直肌分离
        LacPro,         //子宫复旧
        UterInvo,       //催乳
        Func_SUM        //功能数量
    }PosReh_Func_Name_t;
  

    //产后康复说明页控件数据
    typedef struct
    {
        char*       Title;              //标题  
        char*       Text;               //正文
        void*       src;                //图片
        int8_t      Cnt_DiaRecti;       //腹直肌分离治疗剩余次数
        int8_t      Cnt_UterInvo;       //催乳治疗剩余次数
        int8_t      Cnt_LacPro;         //子宫复旧治疗剩余次数
        lv_obj_t*   Btn_DiaRecti;       //腹直肌分离按钮
        lv_obj_t*   Btn_UterInvo;       //催乳按钮
        lv_obj_t*   Btn_LacPro;         //子宫复旧按钮
        lv_obj_t*   Btn_Select;         //选择按钮
        lv_obj_t*   Label_DiaRecti;     //腹直肌分离标签
        lv_obj_t*   Label_UterInvo;     //催乳标签
        lv_obj_t*   Label_LacPro;       //子宫复旧标签
        lv_obj_t*   Label_Select;       //选择标签

    }PosReh_Instr_Data_t;

    //产后康复功能项展示数据
    typedef struct
    {
        lv_obj_t* Obj_Func;       //功能容器
        lv_obj_t* Btn_Func;       //功能按钮
        lv_obj_t* Obj_Select;     //选中控件
        lv_obj_t* Label_Func;     //功能标签
        lv_obj_t* Label_NO;       //标签序号
    }PosReh_SelTreat_t;
    //
 
    typedef struct
    {
        PosReh_Instr_Data_t         InsData;                    //说明页控件数据
        PosReh_SelTreat_t           SelTreat_Data[Func_SUM];    //产后康复功能项展示页控件数据
    }PosReh_Widget_t;

    extern Param_Data_t PosReh_Param_Data1;
    extern Treat_Timer_Ctx_t PosReh_CtxData1;
    uint8_t PosReh_Get_CurFunc(void);
    void PosReh_Set_CurFunc(PosReh_Func_Name_t func);
    void PosReh_Page_ui(void);
    // void PosReh_Show_Page2_widget(void);
    // void PosReh_Show_Page3_widget(TreIns_Data_t* param);
    // void PosReh_Show_Page4_widget(Param_Data_t* param);
    // void PosReh_Show_Page5_widget(Treat_Timer_Ctx_t* param);

    void PosReh_Page_Load(PosReh_PageID_t page_id);

    /* [我们] B4 通用守则清理接口（坑 6.8 / B2-4 / B3-3 同族） */
    void PosReh_LeaveGroup(void);
    void PosReh_StopTimer(void);
    void PosReh_BackToMenu(void);

    /* [我们] 参数存储（NVS）：应用已存治疗参数 / 保存全部治疗参数 */
    void PosReh_Param_ApplySaved(void);
    void PosReh_Param_SaveAll(void);
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
