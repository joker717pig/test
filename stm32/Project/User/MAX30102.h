#ifndef __MAX30102_H
#define __MAX30102_H

#include "stm32f10x.h"

void MAX30102_Init(void);
void max30102_FIFO_ReadBytes(uint8_t* data, uint8_t len);
void maxim_heart_rate_and_oxygen_saturation(void);

#endif
