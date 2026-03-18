#include "MAX30102.h"

void MAX30102_Init(void) {
    // 实现MAX30102初始化
	uint8_t data;
    
	I2C_Bus_Init();   //初始化I2C接口
	delay_ms(500);
    
    max30102_int_gpio_init();   //中断引脚配置
    
	max30102_i2c_write(MODE_CONFIGURATION,0x40);  //reset the device
	
	delay_ms(5);
	
	max30102_i2c_write(INTERRUPT_ENABLE1,0xE0);
	max30102_i2c_write(INTERRUPT_ENABLE2,0x00);  //interrupt enable: FIFO almost full flag, new FIFO Data Ready,
																						 	//                   ambient light cancellation overflow, power ready flag, 
																							//						    		internal temperature ready flag
	
	max30102_i2c_write(FIFO_WR_POINTER,0x00);
	max30102_i2c_write(FIFO_OV_COUNTER,0x00);
	max30102_i2c_write(FIFO_RD_POINTER,0x00);   //clear the pointer
	
	max30102_i2c_write(FIFO_CONFIGURATION,0x4F); //FIFO configuration: sample averaging(4),FIFO rolls on full(0), FIFO almost full value(15 empty data samples when interrupt is issued)  
	
	max30102_i2c_write(MODE_CONFIGURATION,0x03);  //MODE configuration:SpO2 mode
	
	max30102_i2c_write(SPO2_CONFIGURATION,0x2A); //SpO2 configuration:ACD resolution:15.63pA,sample rate control:200Hz, LED pulse width:215 us 
	
	max30102_i2c_write(LED1_PULSE_AMPLITUDE,0x2f);	//IR LED
	max30102_i2c_write(LED2_PULSE_AMPLITUDE,0x2f); //RED LED current
	
	max30102_i2c_write(TEMPERATURE_CONFIG,0x01);   //temp
	
	max30102_i2c_read(INTERRUPT_STATUS1,&data,1);
	max30102_i2c_read(INTERRUPT_STATUS2,&data,1);  //clear status
	
	
}

void max30102_FIFO_ReadBytes(uint8_t* data, uint8_t len) {
    // 实现读取FIFO数据
}

void maxim_heart_rate_and_oxygen_saturation(void) {
    // 实现血氧心率计算
}
