#include "My_VL6180.h"

#define VL6180_DEFAULT_ID 0xB4
#define VL6180_DEFAULT_I2C_ADDR 0x29
#define VL6180_REG_IDENTIFICATION_MODEL_ID 0x000
#define VL6180_REG_SYSTEM_INTERRUPT_CONFIG 0x014
#define VL6180_REG_SYSTEM_INTERRUPT_CLEAR 0x015
#define VL6180_REG_SYSTEM_FRESH_OUT_OF_RESET 0x016
#define VL6180_REG_SYSRANGE_START 0x018
#define VL6180_REG_SYSALS_START 0x038
#define VL6180_REG_SYSALS_ANALOGUE_GAIN 0x03F
#define VL6180_REG_SYSALS_INTEGRATION_PERIOD_HI 0x040
#define VL6180_REG_SYSALS_INTEGRATION_PERIOD_LO 0x041
#define VL6180_REG_RESULT_RANGE_STATUS 0x04d
#define VL6180_REG_RESULT_INTERRUPT_STATUS_GPIO 0x04f
#define VL6180_REG_RESULT_ALS_VAL 0x050
#define VL6180_REG_RESULT_RANGE_VAL 0x062
#define VL6180_ALS_GAIN_1 0x06
#define VL6180_ALS_GAIN_1_25 0x05
#define VL6180_ALS_GAIN_1_67 0x04
#define VL6180_ALS_GAIN_2_5 0x03
#define VL6180_ALS_GAIN_5 0x02
#define VL6180_ALS_GAIN_10 0x01
#define VL6180_ALS_GAIN_20 0x00
#define VL6180_ALS_GAIN_40 0x07

#define VL6180_ERROR_NONE 0
#define VL6180_ERROR_SYSERR_1 1
#define VL6180_ERROR_SYSERR_5 5
#define VL6180_ERROR_ECEFAIL 6
#define VL6180_ERROR_NOCONVERGE 7
#define VL6180_ERROR_RANGEIGNORE 8
#define VL6180_ERROR_SNR 11
#define VL6180_ERROR_RAWUFLOW 12
#define VL6180_ERROR_RAWOFLOW 13
#define VL6180_ERROR_RANGEUFLOW 14
#define VL6180_ERROR_RANGEOFLOW 15

void VL6180_WriteByte(uint16_t reg, uint8_t data)
{
    HAL_I2C_Mem_Write(&hi2c2, (VL6180_DEFAULT_I2C_ADDR << 1) | 0, reg, I2C_MEMADD_SIZE_16BIT, &data, 1, HAL_MAX_DELAY);
}

void VL6180_WriteByte_16Bit(uint16_t reg, uint16_t data)
{
    uint8_t data2[2] = {0, 0};
    data2[0] = data >> 8;
    data2[1] = data;
    HAL_I2C_Mem_Write(&hi2c2, (VL6180_DEFAULT_I2C_ADDR << 1) | 0, reg, I2C_MEMADD_SIZE_16BIT, data2, 2, HAL_MAX_DELAY);
}

uint8_t VL6180_ReadByte(uint16_t reg)
{
    uint8_t data = 0;
    HAL_I2C_Mem_Read(&hi2c2, (VL6180_DEFAULT_I2C_ADDR << 1) | 1, reg, I2C_MEMADD_SIZE_16BIT, &data, 1, HAL_MAX_DELAY);
    return data;
}

uint16_t VL6180_ReadBytee_16Bit(uint16_t reg)
{
    uint16_t data = 0;
    uint8_t data2[2];
    HAL_I2C_Mem_Read(&hi2c2, (VL6180_DEFAULT_I2C_ADDR << 1) | 1, reg, I2C_MEMADD_SIZE_16BIT, data2, 2, HAL_MAX_DELAY);
    data = data2[0];
    data = data << 8;
    data += data2[1];

    return data;
}

uint8_t VL6180_Init()
{
    uint8_t ptp_offset = VL6180_ReadByte(VL6180_REG_SYSTEM_FRESH_OUT_OF_RESET);
    //	if(VL6180_Read_ID(add) == VL6180_DEFAULT_ID)
    if (ptp_offset == 0x01)
    {
        VL6180_WriteByte(0x0207, 0x01);
        VL6180_WriteByte(0x0208, 0x01);
        VL6180_WriteByte(0x0096, 0x00);
        VL6180_WriteByte(0x0097, 0xfd);
        VL6180_WriteByte(0x00e3, 0x00);
        VL6180_WriteByte(0x00e4, 0x04);
        VL6180_WriteByte(0x00e5, 0x02);
        VL6180_WriteByte(0x00e6, 0x01);
        VL6180_WriteByte(0x00e7, 0x03);
        VL6180_WriteByte(0x00f5, 0x02);
        VL6180_WriteByte(0x00d9, 0x05);
        VL6180_WriteByte(0x00db, 0xce);
        VL6180_WriteByte(0x00dc, 0x03);
        VL6180_WriteByte(0x00dd, 0xf8);
        VL6180_WriteByte(0x009f, 0x00);
        VL6180_WriteByte(0x00a3, 0x3c);
        VL6180_WriteByte(0x00b7, 0x00);
        VL6180_WriteByte(0x00bb, 0x3c);
        VL6180_WriteByte(0x00b2, 0x09);
        VL6180_WriteByte(0x00ca, 0x09);
        VL6180_WriteByte(0x0198, 0x01);
        VL6180_WriteByte(0x01b0, 0x17);
        VL6180_WriteByte(0x01ad, 0x00);
        VL6180_WriteByte(0x00ff, 0x05);
        VL6180_WriteByte(0x0100, 0x05);
        VL6180_WriteByte(0x0199, 0x05);
        VL6180_WriteByte(0x01a6, 0x1b);
        VL6180_WriteByte(0x01ac, 0x3e);
        VL6180_WriteByte(0x01a7, 0x1f);
        VL6180_WriteByte(0x0030, 0x00);

        // Recommended : Public registers - See data sheet for more detail
        VL6180_WriteByte(0x0011, 0x10); // Enables polling for 'New Sample ready'
                                        // when measurement completes
        VL6180_WriteByte(0x010a, 0x30); // Set the averaging sample period
                                        // (compromise between lower noise and
                                        // increased execution time)
        VL6180_WriteByte(0x003f, 0x46); // Sets the light and dark gain (upper
                                        // nibble). Dark gain should not be
                                        // changed. !上半字节要写入0x4	默认增益是1.0
        VL6180_WriteByte(0x0031, 0xFF); // sets the # of range measurements after
                                        // which auto calibration of system is
                                        // performed
        VL6180_WriteByte(0x0041, 0x63); // Set ALS integration time to 100ms
        VL6180_WriteByte(0x002e, 0x01); // perform a single temperature calibration
                                        // of the ranging sensor

        // Optional: Public registers - See data sheet for more detail
        VL6180_WriteByte(0x001b, 0x09); // 测量间隔	轮询模式
                                        //  period to 100ms	每步10ms->0-10ms
        VL6180_WriteByte(0x003e, 0x31); // 测量周期	ALS模式
                                        //  to 500ms
        VL6180_WriteByte(0x0014, 0x24); // Configures interrupt on 'New Sample
                                        // Ready threshold event'

        // VL6180_WriteByte(VL6180_REG_SYSTEM_FRESH_OUT_OF_RESET, 0x00); //不发送00那么读出来的数值就是01

        return 0;
    }
    else
        return 1;
}

uint8_t VL6180_Read_ID()
{
    return VL6180_ReadByte(VL6180_REG_IDENTIFICATION_MODEL_ID);
}

uint8_t VL6180_Read_Range()
{
    uint8_t range = 0;
    static uint32_t next = 0; 
    if (HAL_GetTick() < next) 
    {                         
        return range;               
    }                         
    next += 10;
    // 等待设备准备好进行量程测量
    while (!(VL6180_ReadByte(VL6180_REG_RESULT_RANGE_STATUS) & 0x01))
        ; // VL6180_REG_RESULT_RANGE_STATUS，0x0d， 自检0x01连续性测试
    // 开始量程测量
    VL6180_WriteByte(VL6180_REG_SYSRANGE_START, 0x01); // VL6180_REG_SYSRANGE_START,0x018,开启测距，0x01，单次模
    // 轮询到第2位被设置
    while (!(VL6180_ReadByte(VL6180_REG_RESULT_INTERRUPT_STATUS_GPIO) & 0x04))
        ; // VL6180_REG_RESULT_INTERRUPT_STATUS_GPIO,中断等待，0x04，数据等待完毕
    // 读数范围(毫米)
    range = VL6180_ReadByte(VL6180_REG_RESULT_RANGE_VAL); // RESULT__RANGE_VAL,0x062,显示检测长度
    // 清除中断
    VL6180_WriteByte(VL6180_REG_SYSTEM_INTERRUPT_CLEAR, 0x07); // VL6180_REG_SYSTEM_INTERRUPT_CLEAR,0x015,清除状态位
    return range;
}
