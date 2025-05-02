/* USER CODE BEGIN Header */
/*
 * @Author: Julian Zyan
 *          zhouyulin_mail@qq.com
 * @Date: 2025-01-01 13:48:34
 * @LastEditors: Julian Zyan
 *               zhouyulin_mail@qq.com
 * @LastEditTime: 2025-03-23 19:50:52
 * @FilePath: \MDK-ARMd:\Project\Demo\Src\main.c
 * @Description:
 *
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved.
 */
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
#include "i2c.h"
#include "iwdg.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "My_VL6180.h"
#include "stdio.h"
#include "string.h"
#include "My_mpu6050.h"
#include "DL_LN3X.h"
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

char uart1_send_buffer[2];
char uart2_send_buffer[2];
char uart1_receive_buffer[2];
char uart2_receive_buffer[2];
float Yaw, Pitch, Roll;
uint8_t Range = 0;
uint8_t Sub_Node_Switch = 0;
DL_HandleTypeDef dl_module;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* 在main.c中添加数据打包发送函数 */
void Send_Sensor_Data(DL_HandleTypeDef *dl, float pitch, float roll, uint8_t range)
{
  // 创建数据缓冲区（注意总长度不能超过63字节）
  uint8_t tx_buf[9]; // 4+4+1=9 bytes

  // 使用memcpy处理浮点数（避免类型转换问题）
  memcpy(&tx_buf[0], &pitch, 4);
  memcpy(&tx_buf[4], &roll, 4);
  tx_buf[8] = range;

  // 发送到主节点
  DL_SendPacket(dl,
                0x80,   // 源端口（用户端口）
                0x80,   // 目的端口（用户端口）
                0x2003, // 主节点地址
                tx_buf,
                sizeof(tx_buf));
  // 添加发送状态检查
  // HAL_StatusTypeDef status = DL_SendPacket(dl, 0x80, 0x80, 0x2026, tx_buf, sizeof(tx_buf));
}

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
  if (huart->Instance == USART1)
  {
    // 将串口2接收的1字节数据通过串口1发送
    HAL_UART_Transmit(&huart2, (uint8_t *)uart1_receive_buffer, 1, HAL_MAX_DELAY);

    // 重新开启串口2的单字节中断接收
    HAL_UART_Receive_IT(&huart2, (uint8_t *)uart1_receive_buffer, 1);
  }
  if (huart == dl_module.huart)
  {
    DL_UART_RxCpltCallback(&dl_module);
  }
}

void I2C_Scan(void)
{
  printf("Get in\n");
  uint8_t rec;
  uint8_t data[10];
  for (uint8_t address = 0; address < 255; address++)
  {
    /* code */
    rec = HAL_I2C_Mem_Read(&hi2c2, address, 0, I2C_MEMADD_SIZE_8BIT, data, 1, 1000);
    if (rec == HAL_OK)
    {
      /* code */
      printf("Find device Address is %d\n", address);
      // HAL_Delay(300);
      break;
    }
    else
    {
      printf("failed\n");
    }
  }
  printf("Get off\n");
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
  uint8_t led[] = {0xfe, 0x05, 0x80, 0x20, 0x26, 0x20, 0x32, 0xff};
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
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_TIM2_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
  /*使能定时器2中断*/
  HAL_TIM_Base_Start_IT(&htim2);
  // 切换节点
  Sub_Node_Switch = 0;

  // 子节点操作
  Mpu_6050_Init();
  HAL_Delay(10);
  VL6180_Init();
  // HAL_Delay(500);
  // 初始化DL模块
  if (Sub_Node_Switch == 0)
  {
    DL_Init(&dl_module, &huart2, 0x2025);
  }
  else if (Sub_Node_Switch == 1)
  {
    DL_Init(&dl_module, &huart2, 0x2026);
  }
  /**
   * @Code_Description:开启串口接收中断
   * @Creat_Time:2025/03/27 12:09:48
   */
  // HAL_UART_Receive_IT(&huart1, (uint8_t *)uart1_receive_buffer, 1);
  // HAL_UART_Receive_IT(&huart2, (uint8_t *)uart2_receive_buffer, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // 子节点操作
    // 喂狗，不进入reset，否则约每5s复位一次
    HAL_IWDG_Refresh(&hiwdg);

    Mpu_6050_EulerAngle_Measurement();

    Yaw = Mpu_6050_GetYaw();
    Pitch = Mpu_6050_GetPitch();
    Roll = Mpu_6050_GetRoll();

    Range = VL6180_Read_Range();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  } /*  */
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

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
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
}

/* USER CODE BEGIN 4 */
/**
 * @Model_Name:定时器回调函数
 * @Code_Description:每500ms进入一次，每过1s发送一次传感器数据
 * @Author:Julian Zyan
 * @E-Mail:zhouyulin_mail@qq.com
 * @Creat_Time:2025/04/03 21:35:39
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  static uint8_t send_time = 0;
  if (htim == (&htim2))
  {
    // 子节点
    send_time++;
    if (send_time == 2)
    {
      // 替换原来的printf为DL发送
      Send_Sensor_Data(&dl_module, Pitch, Roll, Range);
      send_time = 0;
    }
  }
}
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
