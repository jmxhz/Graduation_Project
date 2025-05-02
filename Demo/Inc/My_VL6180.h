#ifndef MY_VL6180_H
#define MY_VL6180_H

#include "stm32f1xx_hal.h"
#include "i2c.h"

void VL6180_WriteByte(uint16_t reg, uint8_t data);
void VL6180_WriteByte_16Bit(uint16_t reg, uint16_t data);
uint8_t VL6180_ReadByte(uint16_t reg);
uint16_t VL6180_ReadBytee_16Bit(uint16_t reg);
uint8_t VL6180_Init();
uint8_t VL6180_Read_ID();
uint8_t VL6180_Read_Range();

#endif