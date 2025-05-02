/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "iwdg.h"
#include "rtc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "lcd.h"
#include "DL_LN3X.h"
#include "string.h"
#include "stdio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
DL_HandleTypeDef dl_module;
uint8_t chinese[][16] = {
    {0x00, 0x00, 0x3F, 0xF8, 0x21, 0x08, 0x21, 0x08, 0x2F, 0xE8, 0x21, 0x08, 0x21, 0x08, 0x3F, 0xF8},
    {0x20, 0x08, 0x27, 0xC8, 0x24, 0x48, 0x24, 0x48, 0x27, 0xC8, 0x40, 0x08, 0x40, 0x28, 0x80, 0x10}, /*"周",0*/

    {0x00, 0x40, 0x20, 0xA0, 0x11, 0x10, 0x12, 0x08, 0x85, 0xF6, 0x48, 0x00, 0x47, 0xC4, 0x14, 0x54},
    {0x14, 0x54, 0x27, 0xD4, 0xE4, 0x54, 0x24, 0x54, 0x27, 0xD4, 0x24, 0x44, 0x25, 0x54, 0x04, 0x88}, /*"渝",1*/

    {0x01, 0x10, 0x01, 0x10, 0xF9, 0x10, 0x21, 0x10, 0x27, 0xBC, 0x21, 0x10, 0x21, 0x10, 0xFB, 0x38},
    {0x23, 0xB8, 0x25, 0x54, 0x25, 0x54, 0x39, 0x92, 0xE1, 0x10, 0x41, 0x10, 0x01, 0x10, 0x01, 0x10}, /*"琳",2*/

    {0x20, 0x80, 0x20, 0x88, 0x20, 0xB0, 0x3E, 0xC0, 0x20, 0x80, 0x20, 0x84, 0x26, 0x84, 0x38, 0x7C},
    {0x21, 0x00, 0x01, 0x00, 0xFF, 0xFE, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00}, /*"毕",3*/

    {0x04, 0x40, 0x04, 0x40, 0x04, 0x40, 0x04, 0x40, 0x44, 0x44, 0x24, 0x44, 0x24, 0x48, 0x14, 0x48},
    {0x14, 0x50, 0x14, 0x60, 0x04, 0x40, 0x04, 0x40, 0x04, 0x40, 0x04, 0x40, 0xFF, 0xFE, 0x00, 0x00}, /*"业",4*/

    {0x00, 0x00, 0x21, 0xF0, 0x11, 0x10, 0x11, 0x10, 0x01, 0x10, 0x02, 0x0E, 0xF4, 0x00, 0x13, 0xF8},
    {0x11, 0x08, 0x11, 0x10, 0x10, 0x90, 0x14, 0xA0, 0x18, 0x40, 0x10, 0xA0, 0x03, 0x18, 0x0C, 0x06}, /*"设",5*/

    {0x00, 0x40, 0x20, 0x40, 0x10, 0x40, 0x10, 0x40, 0x00, 0x40, 0x00, 0x40, 0xF7, 0xFE, 0x10, 0x40},
    {0x10, 0x40, 0x10, 0x40, 0x10, 0x40, 0x10, 0x40, 0x14, 0x40, 0x18, 0x40, 0x10, 0x40, 0x00, 0x40}, /*"计",6*/
};

RTC_TimeTypeDef currTime;
RTC_DateTypeDef currDate;
char timeBuff[20];
char dateBuff[20];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/**
 * @Model_Name:HAL_UART_RxCpltCallback
 * @Code_Description:接收完成回调
 * @param:*huart 串口标识
 * @param:
 * @Author:Julian Zyan
 * @E-Mail:zhouyulin_mail@qq.com
 * @Creat_Time:2025/03/23 21:08:36
 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == dl_module.huart)
  {
    DL_UART_RxCpltCallback(&dl_module);
  }
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  /* USER CODE BEGIN 1 */
  char lcd_show_addr_1_buffer[24];
  char lcd_show_addr_2_buffer[24];
  char lcd_show_pitch_1_buffer[24];
  char lcd_show_roll_1_buffer[24];
  char lcd_show_range_1_buffer[24];
  char lcd_show_pitch_2_buffer[24];
  char lcd_show_roll_2_buffer[24];
  char lcd_show_range_2_buffer[24];
  char weekdayBuff[24];
  // 将星期几转换为字符串
  const char *weekdays[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_IWDG_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  DL_Init(&dl_module, &huart2, 0x2003);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // 喂狗 4s以内
    HAL_IWDG_Refresh(&hiwdg);

    LCD_Init();//ID 9341

    // 获取RTC时间
    HAL_RTC_GetDate(&hrtc, &currDate, RTC_FORMAT_BIN);
    HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
		
    LCD_Clear(WHITE);
    POINT_COLOR = BLACK;

    LCD_ShowString(10, 10, 300, 36, 24, (uint8_t *)"SMR BY Zhou");

    sprintf(lcd_show_addr_1_buffer, "Addr_1: %04x", DL_LN3X_Get_Addr_1());
    sprintf(lcd_show_addr_2_buffer, "Addr_2: %04x", DL_LN3X_Get_Addr_2());

    sprintf(lcd_show_pitch_1_buffer, "Pitch_1: %.2f", DL_LN3X_Get_Pitch_1());
    sprintf(lcd_show_roll_1_buffer, "Roll_1: %.2f", DL_LN3X_Get_Roll_1());
    sprintf(lcd_show_range_1_buffer, "Range_1: %d", DL_LN3X_Get_Range_1());

    sprintf(lcd_show_pitch_2_buffer, "Pitch_2: %.2f", DL_LN3X_Get_Pitch_2());
    sprintf(lcd_show_roll_2_buffer, "Roll_2: %.2f", DL_LN3X_Get_Roll_2());
    sprintf(lcd_show_range_2_buffer, "Range_2: %d", DL_LN3X_Get_Range_2());

    // 格式化时间字符串
    sprintf(dateBuff, "%04d/%02d/%02d",
            currDate.Year + 2000,
            currDate.Month,
            currDate.Date);

    sprintf(timeBuff, "%02d:%02d:%02d",
            currTime.Hours,
            currTime.Minutes,
            currTime.Seconds);
    
    if (currDate.WeekDay >= 1 && currDate.WeekDay <= 7)
    {
      strcpy(weekdayBuff, weekdays[currDate.WeekDay - 1]);
    }
    else
    {
      strcpy(weekdayBuff, "N/A");
    }

    LCD_ShowString(10, 50, 100, 12, 12, (uint8_t *)lcd_show_addr_1_buffer);
    LCD_ShowString(130, 50, 100, 12, 12, (uint8_t *)lcd_show_addr_2_buffer);

    LCD_ShowString(10, 80, 100, 16, 12, (uint8_t *)lcd_show_pitch_1_buffer);
    LCD_ShowString(10, 110, 100, 16, 12, (uint8_t *)lcd_show_roll_1_buffer);
    LCD_ShowString(10, 140, 100, 16, 12, (uint8_t *)lcd_show_range_1_buffer);

    LCD_ShowString(130, 80, 100, 16, 12, (uint8_t *)lcd_show_pitch_2_buffer);
    LCD_ShowString(130, 110, 100, 16, 12, (uint8_t *)lcd_show_roll_2_buffer);
    LCD_ShowString(130, 140, 100, 16, 12, (uint8_t *)lcd_show_range_2_buffer);

    LCD_ShowString(10, 170, 120, 16, 16, (uint8_t *)dateBuff);
    LCD_ShowString(130, 170, 100, 16, 16, (uint8_t *)timeBuff);
    LCD_ShowString(10, 200, 100, 16, 16, (uint8_t *)weekdayBuff);

    LCD_ShowChinese(10, 230, chinese[0], chinese[1], BLACK, WHITE);
    LCD_ShowChinese(26, 230, chinese[2], chinese[3], BLACK, WHITE);
    LCD_ShowChinese(42, 230, chinese[4], chinese[5], BLACK, WHITE);
    LCD_ShowChinese(58, 230, chinese[6], chinese[7], BLACK, WHITE);
    LCD_ShowChinese(74, 230, chinese[8], chinese[9], BLACK, WHITE);
    LCD_ShowChinese(90, 230, chinese[10], chinese[11], BLACK, WHITE);
    LCD_ShowChinese(106, 230, chinese[12], chinese[13], BLACK, WHITE);

    DL_CheckTimeout(5000);

    HAL_Delay(500);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
