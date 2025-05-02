#ifndef MY_MPU6050_H
#define MY_MPU6050_H

#include "stm32f1xx_hal.h"
#include "i2c.h"

#define Pi 3.1415926f

#define PERIODIC(T)           \
    static uint32_t next = 0; \
    if (HAL_GetTick() < next) \
    {                         \
        return;               \
    }                         \
    next += T;                

void Mpu_6050_Init(void);
void Mpu_6050_Update(void);
void Mpu_6050_EulerAngle_Measurement(void);

float Mpu_6050_GetAx(void);
float Mpu_6050_GetAy(void);
float Mpu_6050_GetAz(void);

float Mpu_6050_GetGx(void);
float Mpu_6050_GetGy(void);
float Mpu_6050_GetGz(void);

float Mpu_6050_GetTemperature(void);

float Mpu_6050_GetYaw(void);
float Mpu_6050_GetPitch(void);
float Mpu_6050_GetRoll(void);

#endif