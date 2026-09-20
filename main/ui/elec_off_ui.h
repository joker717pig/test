/**
  ******************************************************************************
  * @文件名称   elec_off_ui.h
  * @文件描述   全局「电极脱落」提示弹窗（纯提示, 单按钮）
  *            - 任何页面检测到电极脱落都弹（弹窗挂 lv_layer_top, 切页 clean 不到）
  *            - 边沿触发: 状态 0→非0 弹一次; 贴好(回到0)前不再重复弹
  *            - 单「确定」按钮 + ENTER/ESC 关闭; 不停治疗/采集, 仅告知
  *            - 弹窗显示期间冻结 keypad group 焦点, 防方向键把焦点移到页面
  *              按钮误触发（治疗中尤其关键）
  *            状态来源（app_modbus 统一快照）:
  *              非采集期 = 心跳 FC03 读 0x0301;
  *              采集期   = STM32 周期推 CNT=0xFFFE 状态帧。
  ******************************************************************************
  */

#ifndef ELEC_OFF_UI_H
#define ELEC_OFF_UI_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  初始化电极脱落提示监控（创建 LVGL 周期 timer）
 * @note   必须在 LVGL + keypad + Frame_ui 之后、LVGL 任务上下文调用一次
 */
void elec_off_ui_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ELEC_OFF_UI_H */
