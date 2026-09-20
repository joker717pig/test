#ifndef __MENU_UI_H__
#define __MENU_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1

#include "lvgl.h"
#include "basic.h"
/* 同事原装 Windows 路径：由 ui_port/app_fs 'C' 盘驱动翻译到 SPIFFS */
#define     POS_ICON              "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/pos.bin"         //疗程说明图标
#define     Stage_unSel_ICON      "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/Stage_unSel.bin" //选择疗程未选中图标
#define     Stage_Sel_ICON        "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/Stage_Sel.bin"   //选择疗程选中图标
#define     WIRING_DIAG_ICON      "C:/Users/zqt/Desktop/PDJ_LVGL/png/page2/2_4_2.bin"         //接线图
#define     PULSE_ICON            "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/PULSE.bin"       //脉宽图标
#define     SET_STAGE_ICON        "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/SET_STAGE.bin"   //阶段设置图标
#define     FREQUENCY_ICON        "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/PREQUENCY.bin"   //频率图标
#define     PLU_SEL_ICON          "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/Plu_Sel.bin"     //加号选中图标
#define     MIN_SEL_ICON          "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/Min_Sel.bin"     //减号选中图标
#define     PLUUP_SEL_ICON        "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/pluUp_Sel.bin"    //向上加号选中图标
#define     PLUUP_UNSEL_ICON      "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/pluUp_unSel.bin"  //向上加号未选中图标
#define     MINDOWN_SEL_ICON      "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/min_down_Sel.bin"      //向下减号选中图标
#define     MINDOWN_UNSEL_ICON    "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/min_down_unSel.bin"    //向下减号未选中图标
#define     BACK_SEL_ICON         "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/back_Sel.bin"          //返回按钮选中图标
#define     BACK_UNSEL_ICON       "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/back_unSel.bin"        //返回按钮未选中图标
#define     PAUSE_SEL_ICON        "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/pause_Sel.bin"         //暂停按钮选中图标
#define     PAUSE_UNSEL_ICON      "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/pause_unSel.bin"       //暂停按钮未选中图标
#define     START_SEL_ICON         "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/start_Sel.bin"        //开始按钮选中图标
#define     START_UNSEL_ICON       "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/start_unSel.bin"      //开始按钮未选中图标
#define     TIME_ICON              "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/time.bin"             //时间图标
/****==========================曲线图页面ICON===========================****/
#define     VAGINA_ICON            "C:/Users/zqt/Desktop/PDJ_LVGL/png/AssPage/2_1.bin"             //阴道图标
#define     NEXTPRE_ICON           "C:/Users/zqt/Desktop/PDJ_LVGL/png/AssPage/2_2.bin"             //下一节图标
/****==========================线材接线图ICON===========================****/
// #define     DIARECTI_ICON         "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/DiastasisCH.bin"         //腹直肌接线图
// #define     LACPRO_ICON           "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/LacProCH.bin"     //子宫复旧接线图
// #define     UTERINVO_ICON         "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/UterineCH.bin"      //催乳接线图
// #define     EMG_FEEDBACK_ICON     "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/EMGCH.bin"            //肌电反馈接线图
// #define     STIM_ICON             "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/STIMCH.bin"                   //电刺激接线图
// #define     DIARECTI_EN_ICON      "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/DiastasisCH.bin"         //腹直肌英文接线图
// #define     LACPRO_EN_ICON        "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/LacProCH.bin"     //子宫复旧英文接线图
// #define     UTERINVO_EN_ICON      "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/UterineCH.bin"      //催乳英文接线图
// #define     EMG_FEEDBACK_EN_ICON  "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/EMGCH.bin"            //肌电反馈英文接线图
// #define     STIM_EN_ICON          "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/STIMCH.bin"                   //电刺激英文接线图

        //阶段数枚举
    typedef enum {
        STAGE_1 = 0,
        STAGE_2,
        STAGE_3,
        STAGE_4,
        STAGE_MAX
    }Treat_Stage_t;

    /* 页面ID */
    typedef enum {
        PAGE_MENU = 0,      //主菜单页
        PAGE_SETTING,       //设置页
        PAGE_PELFLO_ASS,    //盆底评估页
        PAGE_PELFLO_THE,    //盆底治疗页
        PAGE_POSREH,        //产后康复页
        PAGE_MAX
    }Menu_PageID_t;

    typedef struct {
        int32_t     Origin_Y;   // 控件起始位置
        int32_t     Origin_X;   // 控件起始位置
        int32_t     END_Y;      // 控件结束位置
        lv_obj_t*   Obj;
    }Scroll_Data_t;

    typedef struct {
        lv_obj_t* PelFlo_Ass_Btn;   //盆底评估按钮
        lv_obj_t* PelFlo_The_Btn;   //盆底治疗按钮
        lv_obj_t* Set_Btn;          //设置按钮
        lv_obj_t* PostRec_Btn;      //产后修复按钮
        lv_obj_t* Ass_seleckImg;    //盆底评估选中效果图片
        lv_obj_t* The_seleckImg;    //盆底治疗选中效果图片
        lv_obj_t* PostRec_seleckImg;//盆底评估选中效果图片
        lv_obj_t* Set_seleckImg;    //产后修复选中效果图片
    }Menu_Widget_t;

    extern Menu_Widget_t Menu_Widget;   /* 定义在 menu_ui.c */

    /* 阶段疗程结构体 */
    typedef struct {
        const char* name;          //阶段名称
        uint8_t      total_times;   //疗程总次数
    }Stage_Desc_t;

    /* 治疗点结构体 */
    typedef struct {
        Treat_Stage_t    stage;   // 第几阶段
        uint8_t          time;    // 第几次治疗（1~N）
    }Treat_Point_t;

    /* 阶段疗程部件内数据 */
    typedef struct
    {
        lv_obj_t* obj;
        lv_obj_t* btn;
        lv_obj_t* img;
    }Stage_Data_t;

    typedef enum {
        TIMER_IDLE,
        TIMER_RUNNING,
        TIMER_PAUSED,
        TIMER_END
    } timer_status_t;
    /* ===== [我们] B4 PosReh 共享控件（同事 MenuPage 原装，移植自 EDA_EMG_LVGL_PC\MenuPage\menu_ui.h）===== */
    /*盆底治疗功能说明数据结构体*/
    typedef struct {
        char*       Title;           //标题
        char*       Text;            //正文
        int32_t     Obj_Wid;         //容器宽
        int32_t     Obj_Hig;         //容器长
        uint32_t    Title_Color;     //标题颜色
        lv_obj_t*   Btn;             //按钮
    }Instr_Data_t;

    /*页面返回跳转数据*/
    typedef struct
    {
        lv_my_child_page_date_t* Par_CurPage;         // 父页面当前页
        lv_my_child_page_date_t* Par_BackPage;        // 父页面返回页
        lv_my_child_page_date_t* Chi_BackPage;        // 子页面返回页
        lv_my_child_page_date_t* Chi_CurPage;         // 子页面当前页
    }JumpToBackPage_t;

    /*页面下一页跳转数据*/
    typedef struct
    {
        lv_my_child_page_date_t* Chi_NextPage;        // 子页面下一页页
        lv_my_child_page_date_t* Chi_CurPage;         // 子页面当前页
    }JumpToNextPage_t;

    /*选择疗程界面数据结构体（对齐 PC 新版：加 Stage_Num + [48]，B5 再用）*/
    typedef struct {
        lv_obj_t*       obj;           //模糊遮罩
        uint8_t         Stage_Num;     //阶段数量
        uint32_t        High_Obj;      //主容器高
        int16_t         Pos_Y;         // 选择疗程界面整体Y位置
        uint8_t         Btn_MaxSum;     //按钮数量
        Stage_Data_t    sta[48];       //阶段疗程控件数量
        Stage_Desc_t*   Sta_Table;     //阶段疗程表
        Treat_Point_t   Treat_Point[48];    //当前治疗点
    }Select_Data_t;

    /*选择疗程界面数据结构体2（治疗页用，B5 再用）*/
    typedef struct {
        lv_obj_t* obj;           //模糊遮罩
        uint32_t        High_Obj;      //主容器高
        uint8_t         Btn_MaxSum;     //按钮数量
        Stage_Data_t*  sta;       //阶段疗程控件数量
        Stage_Desc_t* Sta_Table;     //阶段疗程表
        Treat_Point_t   Treat_Point[30];    //当前治疗点
    }Select_Data2_t;

    /*阶段疗程说明数据结构体*/
    typedef struct {
        char*       Title;           //标题
        char*       Title_time;      //治疗次数标题
        char*       Label_Btn1;      //按钮1标签
        char*       Label_Btn2;      //按钮2标签
        uint16_t    page_id;         //页面ID
        int32_t     Obj_Wid;         //容器宽
        int32_t     Obj_Hig;         //容器长
        uint32_t    Title_Color;     //标题颜色
        void*       src;             //图片
        lv_obj_t*   History_Btn;     //历史按钮
        lv_obj_t*   Start_Btn;       //开始治疗按钮
        lv_obj_t*   Obj_Img;         //图片容器
        char* Text;            //正文
        Scroll_Data_t* Scroll;

    }TreIns_Data_t;

    /*电刺激参数设置数据结构体*/
    typedef struct {
        uint8_t     stage_stim;      //电刺激阶段 单/双
        uint16_t    page_id;         //页面ID
        void*       Plu_src;         //按钮加图片
        void*       Min_src;         //按钮减图片
        char*       Text_Set1;        //设置按钮文字
        char*       Text_Set2;        //设置按钮文字
        uint16_t    Value_Pulse;     //脉宽值
        uint16_t    Value_Freq;      //频率值
        int32_t     Value_Intens1;   //阶段1强度值
        int32_t     Value_Intens2;   //阶段2强度值
        int8_t      Value_Stage;     //阶段值
        uint32_t    Slder_Color;     //标题颜色

        lv_obj_t*   Set_Btn;         //设置完成按钮
        lv_obj_t*   Min_Btn;         //减按钮
        lv_obj_t*   Plu_Btn;         //加按钮
        lv_obj_t*   Slider;          //滑动条
        lv_obj_t*   Label_Intens;    //强度标签
        lv_obj_t*   Label_Stage;     //阶段标签
        lv_obj_t*   Label_Pulse;     //脉宽标签
        lv_obj_t*   Label_Freq;      //频率标签
        lv_obj_t*   Label_Set;       //设置按钮标签
        /* [我们] 同事原装 `lv_style_t Style_KnobColor;`（值），初始化却用 `&style_YellowKnob`（指针）→ 改指针 */
        lv_style_t* Style_KnobColor; //滑动条把手颜色样式
    }Param_Data_t;

    /*电刺激治疗数据结构体*/
    typedef struct {
        int32_t     Intens_Value;       //强度值
        int32_t     Value_Pulse;        //脉宽值
        int32_t     Value_Freq;          //频率值
        uint32_t    Color;              //颜色
        void*       Plu_src;            //按钮加图片
        void*       Min_src;            //按钮减图片
        void*       Pause_src;          //暂停图片
        void*       Start_src;          //开始图片
        void*       Back_src;           //返回图片
        lv_obj_t*   Min_Btn;            //减按钮
        lv_obj_t*   Plu_Btn;            //加按钮
        lv_obj_t*   Pause_Btn;          //暂停按钮
        lv_obj_t*   Start_Btn;          //开始按钮
        lv_obj_t*   Back_Btn;           //返回按钮
        lv_obj_t*   Label_Intens;       //强度值标签
        lv_obj_t*   Label_Pause;        //暂停按钮标签
        lv_obj_t*   Label_Start;        //开始按钮标签
        lv_obj_t*   Label_Back;         //返回按钮标签
        lv_obj_t*   Label_RemTime;      //时间显示标签
        lv_obj_t*   Label_Hint;         //时间提示标签
        lv_obj_t*   Label_Pulse;        //[我们] 当前步骤脉宽标签（阶段C 参数切换显示）
        lv_obj_t*   Label_Freq;         //[我们] 当前步骤频率标签（阶段C 参数切换显示）
        lv_obj_t*   Label_stage_stime;  //当前电刺激阶段标签
    }Treat_Data_t;

    /*弹窗控件结构体（对齐 PC 新版：从 Treat_Data_t 拆出）*/
    typedef struct {
        lv_obj_t* Msgbox;             //消息弹窗
        lv_obj_t* Yes_Btn;            //消息弹窗 是-按钮
        lv_obj_t* No_Btn;             //消息弹窗 否-按钮
    }Window_Data_t;

    /*曲线表数据结构体（对齐 PC 新版，B5-B 移植 Chart_widget 用）*/
    typedef struct {
        float       Value_MaxEMG;           //最大肌电位值
        float       Value_InsEMG;           //瞬时肌电位值
        uint32_t    Value_TimeLeft;         //剩余时间 单位ms
        lv_obj_t*   Label_MaxEMG;           //最大肌电位标签
        lv_obj_t*   Label_InsEMG;           //瞬时肌电位标签
        lv_obj_t*   Label_TimeLeft;         //剩余时间标签
        lv_obj_t*   Label_Finish;           //已完成标签
        lv_obj_t*   Title_Vagina;           //阴道标题
        lv_obj_t*   Title_NextPre;          //下一节标题
        lv_timer_t* Timer_Treat;            //治疗定时器
        lv_timer_t* Timer_Back;             //返回定时器
        lv_obj_t*   Chart;                  //曲线表
        lv_obj_t*   Figure;                 //下一节图形
        Window_Data_t Window;               //弹窗控件
        timer_status_t  status;              // 当前状态
        int32_t     tick;                   // [我们] 曲线 tick 计数（ESC 退出后重建页面自动归零）
    }Chart_Data_t;

   

    //倒计时数据上下文（对齐 PC 新版：双 timer + Window，删 Page/Main_Page）
    typedef struct {
        int32_t         total_sec;           // 总秒数 (比如 180秒 = 3分钟)
        int32_t         remain_sec;          // 剩余秒数
        uint32_t        last_tick;           // 上次更新的系统tick
        uint32_t        pause_tick;          // 暂停时的时间戳 (用于无漂移计算)
        timer_status_t  status;              // 当前状态
        Treat_Data_t    treat_data;          // 治疗页的控件
        lv_timer_t*     timer_Treat;         // 治疗剩余时间定时器
        lv_timer_t*     timer_Next;          // 下一节倒计时定时器
        Window_Data_t   Window;              // 窗口控件
    } Treat_Timer_Ctx_t;

    /* [我们] 菜单页子页面（PosReh ctx 的 Par_BackPage/Par_CurPage 引用；定义在 menu_ui.c） */
    extern lv_my_child_page_date_t lv_my_MenPage_t;

    /* 治疗页滚动补偿外部坐标（TreIns_Widget 内注册 scroll 回调使用） */
    extern lv_coord_t ThePage_Obj_origin_y;
    extern lv_coord_t ThePage_Obj_origin_y2;
    extern lv_coord_t ThePage_HisBtn_origin_y;
    extern lv_coord_t ThePage_StrBtn_origin_y;

    extern lv_obj_t *g_guide_cont;     /* 引导波形容器（波形图底层） */
    extern lv_obj_t *g_guide_cont_line;     /* 引导波形容器（波形图底层） */
    extern uint8_t   g_guide_phase;       /* 当前阶段序号（0~3），绘制回调据此画三角峰 */  
    extern uint8_t   last_guide_phase;    /* 上一个阶段序号（0~3），绘制回调据此画三角峰 */
    extern uint8_t chart_repeat;    /*波形图循环次数 */
    extern uint8_t cur_repeat_cnt;
    /* 函数声明 */
    void Menu_ui(lv_obj_t* parent);
    void Menu_Page_Load(Menu_PageID_t page_id);
    void Menu_Show_widget(void);

    /* [我们] B5-C 子页面 stub（定义在 menu_ui.c）：保养/障碍/凯格尔三格占位复用 */
    void Show_StubPage(const char* title);

    /* [我们] B4/B5 共享控件（同事 MenuPage 原装，B5 对齐新版签名） */
    void TreIns_Widget(lv_obj_t* page_cont, TreIns_Data_t* data);
    void ParSet_Widget(lv_obj_t* page_cont, Param_Data_t* data);
    void Treat_Widget(lv_obj_t* page_cont, Treat_Timer_Ctx_t* data);
    void Instr_Widget(lv_obj_t* page_cont, Instr_Data_t* data);
    void Select_Widget2(lv_obj_t* page_cont, Select_Data_t* data);
    void Chart_widget(lv_obj_t* page_cont, Chart_Data_t* widget);
    void Back_Window(Window_Data_t* data, const char* txt);
    void Warn_Window(Window_Data_t* data);
    void Therapy_Guide_Update(uint8_t phase_idx);
    void Chart_Init_Point(lv_obj_t* chart);
    void Therapy_Guide_LineInit(lv_obj_t *parent);
    void Therapy_Guide_Init(lv_obj_t *parent);
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
