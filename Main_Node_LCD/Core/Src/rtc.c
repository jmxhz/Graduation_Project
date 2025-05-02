/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    rtc.c
 * @brief   RTC配置，具有断电恢复后通过秒数计算正确天数的功能
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
#include "rtc.h"
#include "stdio.h"

/* Private defines -----------------------------------------------------------*/
#define ENABLE_BKP_ACCESS()         \
    do                              \
    {                               \
        __HAL_RCC_PWR_CLK_ENABLE(); \
        HAL_PWR_EnableBkUpAccess(); \
        __HAL_RCC_BKP_CLK_ENABLE(); \
    } while (0)

/* 备份寄存器定义 */
#define BKP_REG_INIT_FLAG RTC_BKP_DR1       // 初始化标志寄存器
#define BKP_INIT_MAGIC_NUM 0xA5A5           // 初始化魔法数字
#define BKP_REG_YEAR RTC_BKP_DR2            // 年份备份寄存器
#define BKP_REG_MONTH RTC_BKP_DR3           // 月份备份寄存器
#define BKP_REG_DATE RTC_BKP_DR4            // 日期备份寄存器
#define BKP_REG_WEEKDAY RTC_BKP_DR5         // 星期备份寄存器
#define BKP_REG_LAST_HOUR RTC_BKP_DR6       // 上次备份的小时寄存器
#define BKP_REG_LAST_MINUTE RTC_BKP_DR7     // 上次备份的分钟寄存器
#define BKP_REG_LAST_SECOND RTC_BKP_DR8     // 上次备份的秒寄存器
#define BKP_REG_LAST_TIMESTAMP RTC_BKP_DR9  // 上次备份的时间戳(秒数)低16位
#define BKP_REG_LAST_TIMESTAMP_H RTC_BKP_DR10 // 上次备份的时间戳高16位

/* 私有函数声明 */
static uint8_t BCD2BIN(uint8_t val);
static uint8_t BIN2BCD(uint8_t val);
static void RTC_BackupCriticalData(RTC_HandleTypeDef *hrtc, RTC_DateTypeDef *sDate, RTC_TimeTypeDef *sTime);
static void RTC_RestoreDate(RTC_HandleTypeDef *hrtc, RTC_DateTypeDef *sDate);
static void RTC_IncrementDate(RTC_DateTypeDef *pDate);
static uint8_t RTC_CheckDateUpdateNeeded(RTC_HandleTypeDef *hrtc);
static uint32_t RTC_CalculateElapsedDays(RTC_HandleTypeDef *hrtc, uint32_t elapsedSeconds);

/**
 * @brief BCD码转二进制
 * @param val BCD码值
 * @return 二进制值
 */
static uint8_t BCD2BIN(uint8_t val) {
    return ((val >> 4) * 10U + (val & 0x0F));
}

/**
 * @brief 二进制转BCD码
 * @param val 二进制值
 * @return BCD码值
 */
static uint8_t BIN2BCD(uint8_t val) {
    return (((val / 10U) << 4U) | (val % 10U));
}

/**
 * @brief 备份关键数据到备份寄存器
 * @param hrtc RTC句柄
 * @param sDate 日期结构体
 * @param sTime 时间结构体
 */
static void RTC_BackupCriticalData(RTC_HandleTypeDef *hrtc, RTC_DateTypeDef *sDate, RTC_TimeTypeDef *sTime) {
    ENABLE_BKP_ACCESS();
    
    // 备份日期时间信息
    HAL_RTCEx_BKUPWrite(hrtc, BKP_REG_YEAR, sDate->Year);
    HAL_RTCEx_BKUPWrite(hrtc, BKP_REG_MONTH, sDate->Month);
    HAL_RTCEx_BKUPWrite(hrtc, BKP_REG_DATE, sDate->Date);
    HAL_RTCEx_BKUPWrite(hrtc, BKP_REG_WEEKDAY, sDate->WeekDay);
    HAL_RTCEx_BKUPWrite(hrtc, BKP_REG_LAST_HOUR, sTime->Hours);
    HAL_RTCEx_BKUPWrite(hrtc, BKP_REG_LAST_MINUTE, sTime->Minutes);
    HAL_RTCEx_BKUPWrite(hrtc, BKP_REG_LAST_SECOND, sTime->Seconds);
    
    // 计算并备份当前时间戳(从当天0点开始的秒数)
    uint32_t currentTimestamp = BCD2BIN(sTime->Hours) * 3600 + 
                               BCD2BIN(sTime->Minutes) * 60 + 
                               BCD2BIN(sTime->Seconds);
    
    // 将时间戳分成高低16位存储
    HAL_RTCEx_BKUPWrite(hrtc, BKP_REG_LAST_TIMESTAMP, currentTimestamp & 0xFFFF);
    HAL_RTCEx_BKUPWrite(hrtc, BKP_REG_LAST_TIMESTAMP_H, (currentTimestamp >> 16) & 0xFFFF);
}

/**
 * @brief 从备份寄存器恢复日期
 * @param hrtc RTC句柄
 * @param sDate 日期结构体
 */
static void RTC_RestoreDate(RTC_HandleTypeDef *hrtc, RTC_DateTypeDef *sDate) {
    ENABLE_BKP_ACCESS();
    sDate->Year = HAL_RTCEx_BKUPRead(hrtc, BKP_REG_YEAR);
    sDate->Month = HAL_RTCEx_BKUPRead(hrtc, BKP_REG_MONTH);
    sDate->Date = HAL_RTCEx_BKUPRead(hrtc, BKP_REG_DATE);
    sDate->WeekDay = HAL_RTCEx_BKUPRead(hrtc, BKP_REG_WEEKDAY);
}

/**
 * @brief 增加日期(处理月份和闰年)
 * @param pDate 日期结构体指针
 */
static void RTC_IncrementDate(RTC_DateTypeDef *pDate) {
    uint8_t year = BCD2BIN(pDate->Year);
    uint8_t month = BCD2BIN(pDate->Month);
    uint8_t day = BCD2BIN(pDate->Date);
    uint8_t weekday = pDate->WeekDay;

    // 计算当前月份的最大天数
    uint8_t max_day;
    if (month == 2) {
        uint16_t fullYear = year + 2000;
        max_day = ((fullYear % 4U == 0) && (fullYear % 100U != 0)) || (fullYear % 400U == 0) ? 29 : 28;
    }
    else if ((month == 4) || (month == 6) || (month == 9) || (month == 11)) {
        max_day = 30;
    }
    else {
        max_day = 31;
    }

    // 增加天数
    day++;
    if (day > max_day) {
        day = 1;
        month++;
        if (month > 12) {
            month = 1;
            year++;
        }
    }

    // 增加星期(1-7循环)
    weekday = (weekday % 7) + 1;

    // 转换回BCD格式
    pDate->Year = BIN2BCD(year);
    pDate->Month = BIN2BCD(month);
    pDate->Date = BIN2BCD(day);
    pDate->WeekDay = weekday;
}

/**
 * @brief 检查是否需要更新日期(跨日检查)
 * @param hrtc RTC句柄
 * @return 1需要更新,0不需要
 */
static uint8_t RTC_CheckDateUpdateNeeded(RTC_HandleTypeDef *hrtc) {
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    uint8_t lastHour, lastMinute, lastSecond, lastDate;

    ENABLE_BKP_ACCESS();
    
    // 读取备份的时间信息
    lastHour = HAL_RTCEx_BKUPRead(hrtc, BKP_REG_LAST_HOUR);
    lastMinute = HAL_RTCEx_BKUPRead(hrtc, BKP_REG_LAST_MINUTE);
    lastSecond = HAL_RTCEx_BKUPRead(hrtc, BKP_REG_LAST_SECOND);
    lastDate = HAL_RTCEx_BKUPRead(hrtc, BKP_REG_DATE);

    if (HAL_RTC_GetTime(hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK ||
        HAL_RTC_GetDate(hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) {
        Error_Handler();
    }

    // 检查是否跨过午夜(当前时间小于上次备份时间)
    uint8_t currentHour = BCD2BIN(sTime.Hours);
    uint8_t currentMinute = BCD2BIN(sTime.Minutes);
    uint8_t currentSecond = BCD2BIN(sTime.Seconds);
    
    uint8_t lastHourBin = BCD2BIN(lastHour);
    uint8_t lastMinuteBin = BCD2BIN(lastMinute);
    uint8_t lastSecondBin = BCD2BIN(lastSecond);
    
    // 比较时间判断是否跨日
    if (currentHour < lastHourBin || 
       (currentHour == lastHourBin && currentMinute < lastMinuteBin) ||
       (currentHour == lastHourBin && currentMinute == lastMinuteBin && currentSecond < lastSecondBin)) {
        return 1;
    }
    
    return 0;
}

/**
 * @brief 计算经过的天数(基于秒数)
 * @param hrtc RTC句柄
 * @param elapsedSeconds 经过的秒数
 * @return 经过的天数
 */
static uint32_t RTC_CalculateElapsedDays(RTC_HandleTypeDef *hrtc, uint32_t elapsedSeconds) {
    // 读取上次备份的时间戳
    ENABLE_BKP_ACCESS();
    uint32_t lastTimestamp = HAL_RTCEx_BKUPRead(hrtc, BKP_REG_LAST_TIMESTAMP);
    lastTimestamp |= (HAL_RTCEx_BKUPRead(hrtc, BKP_REG_LAST_TIMESTAMP_H) << 16);
    
    // 计算总秒数(包括上次时间戳和经过的秒数)
    uint32_t totalSeconds = lastTimestamp + elapsedSeconds;
    
    // 计算完整的天数
    return totalSeconds / 86400;  // 86400秒=1天
}

/* 回调函数 ----------------------------------------------------------------*/
/**
 * @brief RTC秒中断回调函数
 * @param hrtc RTC句柄
 */
void HAL_RTCEx_SecondEventCallback(RTC_HandleTypeDef *hrtc) {
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    static uint8_t prevSec = 0;

    if (HAL_RTC_GetTime(hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK ||
        HAL_RTC_GetDate(hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) {
        Error_Handler();
    }

    uint8_t currentSec = BCD2BIN(sTime.Seconds);

    // 检查是否跨过分钟(用于减少备份频率)
    if (currentSec == 0) {
        // 每分钟备份一次关键数据
        RTC_BackupCriticalData(hrtc, &sDate, &sTime);
    }

    // 检查是否跨日(秒从59变为0)
    if (prevSec == 59 && currentSec == 0) {
        RTC_IncrementDate(&sDate);
        if (HAL_RTC_SetDate(hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) {
            Error_Handler();
        }
        // 跨日后立即备份
        RTC_BackupCriticalData(hrtc, &sDate, &sTime);
    }

    prevSec = currentSec;
}

/* 初始化函数 -----------------------------------------------------------*/
RTC_HandleTypeDef hrtc;

/**
 * @brief RTC初始化函数
 */
void MX_RTC_Init(void) {
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    ENABLE_BKP_ACCESS();

    /** 初始化RTC */
    hrtc.Instance = RTC;
    hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
    hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        Error_Handler();
    }

    /* 检查RTC是否需要初始化 */
    if (HAL_RTCEx_BKUPRead(&hrtc, BKP_REG_INIT_FLAG) != BKP_INIT_MAGIC_NUM) {
        // 初始时间设置
        sTime.Hours = 0x17;
        sTime.Minutes = 0x46;
        sTime.Seconds = 0x0;
        if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK) {
            Error_Handler();
        }

        // 初始日期设置
        sDate.Date = 0x30;
        sDate.Month = RTC_MONTH_APRIL;
        sDate.Year = 0x25;
        sDate.WeekDay = RTC_WEEKDAY_TUESDAY;
        if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) {
            Error_Handler();
        }

        // 写入初始化标志和备份数据
        HAL_RTCEx_BKUPWrite(&hrtc, BKP_REG_INIT_FLAG, BKP_INIT_MAGIC_NUM);
        RTC_BackupCriticalData(&hrtc, &sDate, &sTime);
    }
    else {
        // 从备份寄存器恢复日期
        RTC_RestoreDate(&hrtc, &sDate);
        if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) {
            Error_Handler();
        }

        // 检查是否需要更新日期(断电后恢复)
        if (RTC_CheckDateUpdateNeeded(&hrtc)) {
            // 获取当前时间计算经过的秒数
            if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BCD) == HAL_OK) {
                // 计算从上次备份到现在的秒数
                uint32_t elapsedSeconds = BCD2BIN(sTime.Hours) * 3600 + 
                                        BCD2BIN(sTime.Minutes) * 60 + 
                                        BCD2BIN(sTime.Seconds);
                
                // 计算经过的天数
                uint32_t elapsedDays = RTC_CalculateElapsedDays(&hrtc, elapsedSeconds);
                
                // 根据经过的天数增加日期
                for (uint32_t i = 0; i < elapsedDays; i++) {
                    RTC_IncrementDate(&sDate);
                }
                
                // 更新RTC日期
                if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) {
                    Error_Handler();
                }
                
                // 备份更新后的数据
                RTC_BackupCriticalData(&hrtc, &sDate, &sTime);
            }
        }
    }

    // 启用秒中断
    __HAL_RTC_SECOND_ENABLE_IT(&hrtc, RTC_IT_SEC);
}

/**
 * @brief RTC MSP初始化
 * @param rtcHandle RTC句柄
 */
void HAL_RTC_MspInit(RTC_HandleTypeDef *rtcHandle) {
    if (rtcHandle->Instance == RTC) {
        ENABLE_BKP_ACCESS();
        __HAL_RCC_RTC_ENABLE();
        HAL_NVIC_SetPriority(RTC_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(RTC_IRQn);
    }
}
