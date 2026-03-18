
#include <stm32f10x.h>
#include "LED.h"


// 初始化程序区
// void GPIO_LED_Configuration(void)
// 函数功能: LED GPIO配置
// 调用模块: RCC_APB2PeriphClockCmd();GPIO_Init();

void GPIO_LED_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  // 使能IO口时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

  // LED灯PE4,PE5配置
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  // 推挽输出
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  // 先关闭两个LED
  LED1_OFF;
  LED2_OFF;
}

