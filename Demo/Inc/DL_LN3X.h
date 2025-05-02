/* DL_LN3X.h */
#ifndef DL_LN3X_H
#define DL_LN3X_H

#include "stm32f1xx_hal.h"

#define DL_PACKET_HEADER   0xFE
#define DL_PACKET_FOOTER   0xFF
#define DL_ESCAPE_CHAR     0xFE
#define DL_ESCAPE_FE       0xFC
#define DL_ESCAPE_FF       0xFD

// 端口定义（根据协议文档）
typedef enum {
    DL_PORT_LED_CTRL    = 0x20,
    DL_PORT_CONFIG      = 0x21,
    DL_PORT_ERROR       = 0x22,
    DL_PORT_RSSI        = 0x23,
    DL_PORT_GPIO4       = 0x44,
    DL_PORT_GPIO5       = 0x45
} DL_Port;

typedef struct {
    UART_HandleTypeDef *huart;
    uint16_t self_addr;
    uint8_t rx_buffer[128];
    uint8_t tx_buffer[128];
    uint16_t rx_index;
} DL_HandleTypeDef;

// 初始化函数
void DL_Init(DL_HandleTypeDef *hdl, UART_HandleTypeDef *huart, uint16_t addr);

// 基础通信函数
HAL_StatusTypeDef DL_SendPacket(DL_HandleTypeDef *hdl, 
                               DL_Port src_port,
                               DL_Port dest_port,
                               uint16_t target_addr,
                               uint8_t *data,
                               uint8_t data_len);

// 配置命令函数
HAL_StatusTypeDef DL_SetAddress(DL_HandleTypeDef *hdl, uint16_t new_addr);

// 实用功能函数
HAL_StatusTypeDef DL_BlinkLED(DL_HandleTypeDef *hdl, uint16_t target_addr, uint8_t duration);

// 接收处理函数
void DL_UART_RxCpltCallback(DL_HandleTypeDef *hdl);

#endif
