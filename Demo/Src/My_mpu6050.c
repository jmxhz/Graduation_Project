#include "My_mpu6050.h"
#include "math.h"

float Ax, Ay, Az;
float Gx, Gy, Gz;
float Temperature;

static float Yaw, Pitch, Roll;

uint8_t Mpu_6050_Address = 0xd0;
uint8_t Mpu_6050_PowerManagement_Address = 0x6b;
uint8_t Mpu_6050_PowerManagement_Reset = 0x80;
uint8_t Mpu_6050_PowerManagement_Quit_Sleep = 0x00;

uint8_t Mpu_6050_Gyro_Config_Address = 0x1b;
uint8_t Mpu_6050_Gyro_FS_SEL_250 = 0x00;
uint8_t Mpu_6050_Gyro_FS_SEL_500 = 0x01;
uint8_t Mpu_6050_Gyro_FS_SEL_1000 = 0x10;
uint8_t Mpu_6050_Gyro_FS_SEL_2000 = 0x11;

uint8_t Mpu_6050_Accel_Config_Address = 0x1c;
uint8_t Mpu_6050_Accel_AFS_SEL_2g = 0x00;
uint8_t Mpu_6050_Accel_AFS_SEL_4g = 0x01;
uint8_t Mpu_6050_Accel_AFS_SEL_8g = 0x10;
uint8_t Mpu_6050_Accel_AFS_SEL_16g = 0x11;

uint8_t Mpu_6050_Ax_H_Address = 0x3b;
uint8_t Mpu_6050_Ax_L_Address = 0x3c;

uint8_t Mpu_6050_Ay_H_Address = 0x3d;
uint8_t Mpu_6050_Ay_L_Address = 0x3e;

uint8_t Mpu_6050_Az_H_Address = 0x3f;
uint8_t Mpu_6050_Az_L_Address = 0x40;

uint8_t Mpu_6050_Temperature_H_Address = 0x41;
uint8_t Mpu_6050_Temperature_L_Address = 0x42;

uint8_t Mpu_6050_Gx_H_Address = 0x43;
uint8_t Mpu_6050_Gx_L_Address = 0x44;

uint8_t Mpu_6050_Gy_H_Address = 0x45;
uint8_t Mpu_6050_Gy_L_Address = 0x46;

uint8_t Mpu_6050_Gz_H_Address = 0x47;
uint8_t Mpu_6050_Gz_L_Address = 0x48;

void Mpu_6050_Init(void)
{
    /**
     * @Code_Description:复位
     * @Creat_Time:2025/03/23 21:12:34
     */
    HAL_I2C_Mem_Write(&hi2c1, Mpu_6050_Address, Mpu_6050_PowerManagement_Address, I2C_MEMADD_SIZE_8BIT, &Mpu_6050_PowerManagement_Reset, 1, HAL_MAX_DELAY);
    HAL_Delay(100);

    /**
     * @Code_Description:退出睡眠模式
     * @Creat_Time:2025/03/23 21:13:42
     */
    HAL_I2C_Mem_Write(&hi2c1, Mpu_6050_Address, Mpu_6050_PowerManagement_Address, I2C_MEMADD_SIZE_8BIT, &Mpu_6050_PowerManagement_Quit_Sleep, 1, HAL_MAX_DELAY);

    /**
     * @Code_Description:设置陀螺仪量程为+-2000°/s
     * @Creat_Time:2025/03/24 12:25:07
     */
    HAL_I2C_Mem_Write(&hi2c1, Mpu_6050_Address, Mpu_6050_Gyro_Config_Address, I2C_MEMADD_SIZE_8BIT, &Mpu_6050_Gyro_FS_SEL_2000, 1, HAL_MAX_DELAY);

    /**
     * @Code_Description: 设置加速度计量程为+-2g
     * @Creat_Time:2025/03/24 12:26:19
     */
    HAL_I2C_Mem_Write(&hi2c1, Mpu_6050_Address, Mpu_6050_Accel_Config_Address, I2C_MEMADD_SIZE_8BIT, &Mpu_6050_Accel_AFS_SEL_2g, 1, HAL_MAX_DELAY);
}


void Mpu_6050_Update(void)
{

    uint8_t Ax_H_Raw[1];
    uint8_t Ax_L_Raw[1];

    uint8_t Ay_H_Raw[1];
    uint8_t Ay_L_Raw[1];

    uint8_t Az_H_Raw[1];
    uint8_t Az_L_Raw[1];

    uint8_t Gx_H_Raw[1];
    uint8_t Gx_L_Raw[1];

    uint8_t Gy_H_Raw[1];
    uint8_t Gy_L_Raw[1];

    uint8_t Gz_H_Raw[1];
    uint8_t Gz_L_Raw[1];

    uint8_t Temperature_H_Raw[1];
    uint8_t Temperature_L_Raw[1];

    int16_t Ax_Raw, Ay_Raw, Az_Raw, Gx_Raw, Gy_Raw, Gz_Raw, Temperature_Raw;

    /**
     * @Code_Description:读取Ax、Ay、Az的原始数据
     * @Creat_Time:2025/03/24 12:35:43
     */
    HAL_I2C_Mem_Read(&hi2c1, Mpu_6050_Address, Mpu_6050_Ax_H_Address, I2C_MEMADD_SIZE_8BIT, Ax_H_Raw, 1, HAL_MAX_DELAY);
    HAL_I2C_Mem_Read(&hi2c1, Mpu_6050_Address, Mpu_6050_Ax_L_Address, I2C_MEMADD_SIZE_8BIT, Ax_L_Raw, 1, HAL_MAX_DELAY);

    HAL_I2C_Mem_Read(&hi2c1, Mpu_6050_Address, Mpu_6050_Ay_H_Address, I2C_MEMADD_SIZE_8BIT, Ay_H_Raw, 1, HAL_MAX_DELAY);
    HAL_I2C_Mem_Read(&hi2c1, Mpu_6050_Address, Mpu_6050_Ay_L_Address, I2C_MEMADD_SIZE_8BIT, Ay_L_Raw, 1, HAL_MAX_DELAY);

    HAL_I2C_Mem_Read(&hi2c1, Mpu_6050_Address, Mpu_6050_Az_H_Address, I2C_MEMADD_SIZE_8BIT, Az_H_Raw, 1, HAL_MAX_DELAY);
    HAL_I2C_Mem_Read(&hi2c1, Mpu_6050_Address, Mpu_6050_Az_L_Address, I2C_MEMADD_SIZE_8BIT, Az_L_Raw, 1, HAL_MAX_DELAY);

    /**
     * @Code_Description:读取Gx、Gy、Gz的原始数据
     * @Creat_Time:2025/03/24 12:38:30
     */
    HAL_I2C_Mem_Read(&hi2c1, Mpu_6050_Address, Mpu_6050_Gx_H_Address, I2C_MEMADD_SIZE_8BIT, Gx_H_Raw, 1, HAL_MAX_DELAY);
    HAL_I2C_Mem_Read(&hi2c1, Mpu_6050_Address, Mpu_6050_Gx_L_Address, I2C_MEMADD_SIZE_8BIT, Gx_L_Raw, 1, HAL_MAX_DELAY);

    HAL_I2C_Mem_Read(&hi2c1, Mpu_6050_Address, Mpu_6050_Gy_H_Address, I2C_MEMADD_SIZE_8BIT, Gy_H_Raw, 1, HAL_MAX_DELAY);
    HAL_I2C_Mem_Read(&hi2c1, Mpu_6050_Address, Mpu_6050_Gy_L_Address, I2C_MEMADD_SIZE_8BIT, Gy_L_Raw, 1, HAL_MAX_DELAY);

    HAL_I2C_Mem_Read(&hi2c1, Mpu_6050_Address, Mpu_6050_Gz_H_Address, I2C_MEMADD_SIZE_8BIT, Gz_H_Raw, 1, HAL_MAX_DELAY);
    HAL_I2C_Mem_Read(&hi2c1, Mpu_6050_Address, Mpu_6050_Gz_L_Address, I2C_MEMADD_SIZE_8BIT, Gz_L_Raw, 1, HAL_MAX_DELAY);

    /**
     * @Code_Description:读取Temperature的原始数据
     * @Creat_Time:2025/03/24 16:51:39
     */
    HAL_I2C_Mem_Read(&hi2c1, Mpu_6050_Address, Mpu_6050_Temperature_H_Address, I2C_MEMADD_SIZE_8BIT, Temperature_H_Raw, 1, HAL_MAX_DELAY);
    HAL_I2C_Mem_Read(&hi2c1, Mpu_6050_Address, Mpu_6050_Temperature_L_Address, I2C_MEMADD_SIZE_8BIT, Temperature_L_Raw, 1, HAL_MAX_DELAY);

    /**
     * @Code_Description:组合为完整的原始数据
     * @Creat_Time:2025/03/24 16:52:51
     */
    Ax_Raw = (int16_t)((uint16_t)(Ax_H_Raw[0] << 8) | Ax_L_Raw[0]);
    Ay_Raw = (int16_t)((uint16_t)(Ay_H_Raw[0] << 8) | Ay_L_Raw[0]);
    Az_Raw = (int16_t)((uint16_t)(Az_H_Raw[0] << 8) | Az_L_Raw[0]);

    Gx_Raw = (int16_t)((uint16_t)(Gx_H_Raw[0] << 8) | Gx_L_Raw[0]);
    Gy_Raw = (int16_t)((uint16_t)(Gy_H_Raw[0] << 8) | Gy_L_Raw[0]);
    Gz_Raw = (int16_t)((uint16_t)(Gz_H_Raw[0] << 8) | Gz_L_Raw[0]);

    Temperature_Raw = (int16_t)((uint16_t)(Temperature_H_Raw[0] << 8) | Temperature_L_Raw[0]);

    Ax = Ax_Raw * 6.1035e-5f;
    Ay = Ay_Raw * 6.1035e-5f;
    Az = Az_Raw * 6.1035e-5f;

    Gx = Gx_Raw * 6.1035e-2f;
    Gy = Gy_Raw * 6.1035e-2f;
    Gz = Gz_Raw * 6.1035e-2f;

    Temperature = Temperature_Raw / 340 + 36.53;
}

/**
 * @Code_Description:欧拉角计算
 * @Creat_Time:2025/03/26 20:27:38
 */
void Mpu_6050_EulerAngle_Measurement(void)
{
    PERIODIC(5)

    Mpu_6050_Update();

    // 初始化偏航角(Yaw)为0.0
    Yaw = 0.0f;
    // 计算陀螺仪积分后的偏航角(Yaw_G)，基于当前Yaw和Z轴角速度(Gz)乘以时间步长0.05
    // float Yaw_G = Yaw + Gz * 0.05f;
    // 计算陀螺仪积分后的俯仰角(Pitch_G)，基于当前Pitch和X轴角速度(Gx)乘以时间步长0.05
    float Pitch_G = Pitch + Gx * 0.05f;
    // 计算陀螺仪积分后的横滚角(Roll_G)，基于当前Roll和Y轴角速度(Gy)乘以时间步长0.05
    float Roll_G = Roll - Gy * 0.05f;

    // 是已校准的加速度计数据
    float Yaw_A = atan2f(Ax, Ay) / Pi * 180.0f; // 使用atan2f提高精度
    float Pitch_A = atan2f(Ay, Az) / Pi * 180.0f;
    float Roll_A = atan2f(Ax, Az) / Pi * 180.0f;

    Yaw = Yaw;
    Pitch = 0.95238 * Pitch_G + (1 - 0.95238) * Pitch_A;
    Roll = 0.95238 * Roll_G + (1 - 0.95238) * Roll_A;
}

/**
 * @Code_Description:返回Ax、Ay、Az
 * @Creat_Time:2025/03/24 17:26:55
 */
float Mpu_6050_GetAx(void)
{
    return Ax;
}

float Mpu_6050_GetAy(void)
{
    return Ay;
}

float Mpu_6050_GetAz(void)
{
    return Az;
}

/**
 * @Code_Description:返回Gx、Gy、Gz
 * @Creat_Time:2025/03/24 17:27:25
 */
float Mpu_6050_GetGx(void)
{
    return Gx;
}

float Mpu_6050_GetGy(void)
{
    return Gy;
}

float Mpu_6050_GetGz(void)
{
    return Gz;
}

/**
 * @Code_Description:返回Temperature
 * @Creat_Time:2025/03/24 17:28:01
 */
float Mpu_6050_GetTemperature(void)
{
    return Temperature;
}

/**
 * @Code_Description:返回Yaw、Pitch、Roll
 * @Creat_Time:2025/03/25 17:14:45
 */
float Mpu_6050_GetYaw(void)
{
    return Yaw;
}

float Mpu_6050_GetPitch(void)
{
    return Pitch;
}

float Mpu_6050_GetRoll(void)
{
    return Roll;
}