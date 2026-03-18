#include <stm32f10x.h>
#include "LED.h"
#include "Delay.h"
#include "lze_lcd.h"
#include "usart.h"	
#include "ADS1292.h"	
#include "spi.h"
#include "EXTInterrupt.h"
#include "PeripheralInit.h"

// 功能程序区
// void PeripheralInit(void)
// 函数功能: 系统外设初始化

void PeripheralInit(void)
{
	Delay_5ms(200);
//  LCD_Init();               		// 液晶初始化
  GPIO_LED_Configuration(); 			// LED初始化
	uart_init(115200);							// 串口初始化
	GPIO_ADS1292_Configuration();		// ADS1292引脚初始化
	EXTInterrupt_Init(); 						// 外部中断初始化
	SPI1_Init();										// SPI1初始化
  ADS1292_PowerOnInit();					// ADS1292上电初始化
}


