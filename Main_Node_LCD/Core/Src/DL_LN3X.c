/* DL_LN3X.c - 数据链路层通信协议实现，支持数据转义、地址配置和LED控制 */
#include "DL_LN3X.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

// 全局变量
uint16_t lcd_show_addr, lcd_show_addr_1, lcd_show_addr_2;
float lcd_show_pitch_1, lcd_show_roll_1, lcd_show_pitch_2, lcd_show_roll_2;
uint8_t lcd_show_range_1, lcd_show_range_2;
char lcd_show_addr_buffer[24];

// 修改：为每个地址单独记录最后接收时间
typedef struct
{
    uint16_t addr;
    uint32_t last_receive_time;
} AddrTimeoutInfo;

AddrTimeoutInfo addr_timeouts[2] = {
    {0x2025, 0}, // 地址1
    {0x2026, 0}  // 地址2
};

/**
 * @brief 初始化数据链路层句柄
 * @param hdl 数据链路层句柄指针
 * @param huart UART句柄指针
 * @param addr 本地设备地址
 */
void DL_Init(DL_HandleTypeDef *hdl, UART_HandleTypeDef *huart, uint16_t addr)
{
    hdl->huart = huart;    // 绑定UART硬件接口
    hdl->self_addr = addr; // 设置本地地址
    hdl->rx_index = 0;     // 接收缓冲区索引归零

    // 初始化全局变量（无数据时的默认值）
    lcd_show_addr = 0x00;
    lcd_show_addr_1 = 0x00;
    lcd_show_pitch_1 = 0.00f;
    lcd_show_roll_1 = 0.00f;
    lcd_show_range_1 = 0;
    lcd_show_addr_2 = 0x00;
    lcd_show_pitch_2 = 0.00f;
    lcd_show_roll_2 = 0.00f;
    lcd_show_range_2 = 0;

    // 初始化超时信息
    addr_timeouts[0].addr = 0x2025;
    addr_timeouts[0].last_receive_time = 0;
    addr_timeouts[1].addr = 0x2026;
    addr_timeouts[1].last_receive_time = 0;

    // 启动UART接收中断（每次接收1字节）
    HAL_UART_Receive_IT(hdl->huart, &hdl->rx_buffer[hdl->rx_index], 1);
}

/**
 * @brief 发送数据包（完整协议封装）
 * @param hdl 数据链路层句柄指针
 * @param src_port 源端口（0x80表示主机）
 * @param dest_port 目标端口（见DL_Port枚举）
 * @param target_addr 目标设备地址（0x0000表示广播）
 * @param data 数据缓冲区指针
 * @param data_len 数据长度（最大120字节）
 * @return HAL_StatusTypeDef HAL库状态码
 */
HAL_StatusTypeDef DL_SendPacket(DL_HandleTypeDef *hdl,
                                DL_Port src_port,
                                DL_Port dest_port,
                                uint16_t target_addr,
                                uint8_t *data,
                                uint8_t data_len)
{
    // 增加长度校验（协议限制）
    if (data_len > 59)
    { // 63 - 4(header) = 59
        return HAL_ERROR;
    }

    uint8_t raw[128]; // 原始协议包缓冲区
    // uint8_t escaped[256]; // 转义后缓冲区（最大可能扩容2倍）

    /* 协议包结构：包头(1) + 长度(1) + 源端口(1) + 目标端口(1) + 地址(2) + 数据(N) + 包尾(1) */
    uint8_t pkt_len = data_len + 4;         // 计算协议字段总长度
    raw[0] = DL_PACKET_HEADER;              // 协议包头
    raw[1] = pkt_len;                       // 长度字段
    raw[2] = src_port;                      // 源端口
    raw[3] = dest_port;                     // 目标端口
    raw[4] = (uint8_t)(target_addr & 0xFF); // 地址低字节（小端序）
    raw[5] = (uint8_t)(target_addr >> 8);   // 地址高字节
    memcpy(&raw[6], data, data_len);        // 拷贝用户数据
    raw[6 + data_len] = DL_PACKET_FOOTER;   // 协议包尾

    // 转义处理（规避包头/包尾/转义字符）
    // escape_data(raw, escaped, 7 + data_len);

    // 通过UART发送数据（HAL_MAX_DELAY表示阻塞式发送）
    return HAL_UART_Transmit(hdl->huart, raw, 7 + data_len, HAL_MAX_DELAY);
}

/**
 * @brief 设置设备地址（需目标设备支持配置模式）
 * @param hdl 数据链路层句柄指针
 * @param new_addr 新设备地址（0x0001-0xFFFE）
 * @return HAL_StatusTypeDef HAL库状态码
 */
HAL_StatusTypeDef DL_SetAddress(DL_HandleTypeDef *hdl, uint16_t new_addr)
{
    // 配置命令：0x11 + 地址低字节 + 地址高字节
    uint8_t cmd[] = {0x11, (uint8_t)(new_addr & 0xFF), (uint8_t)(new_addr >> 8)};
    // 发送到配置端口（目标地址0x0000表示广播）
    return DL_SendPacket(hdl, 0x80, DL_PORT_CONFIG, 0x0000, cmd, sizeof(cmd));
}

/**
 * @brief 控制目标设备的LED闪烁（需设备支持LED控制指令）
 * @param hdl 数据链路层句柄指针
 * @param target_addr 目标设备地址
 * @param duration LED闪烁持续时间（单位：100ms）
 * @return HAL_StatusTypeDef HAL库状态码
 */
HAL_StatusTypeDef DL_BlinkLED(DL_HandleTypeDef *hdl, uint16_t target_addr, uint8_t duration)
{
    uint8_t cmd[] = {duration}; // 单字节控制指令
    return DL_SendPacket(hdl, 0x80, DL_PORT_LED_CTRL, target_addr, cmd, sizeof(cmd));
}

/**
 * @brief 检查接收超时并重置变量（需在主循环中调用）
 * @param timeout_ms 超时阈值（毫秒）
 */
void DL_CheckTimeout(uint32_t timeout_ms)
{
    uint32_t current_time = HAL_GetTick();

    // 检查地址1 (0x2025) 是否超时
    if (addr_timeouts[0].last_receive_time > 0 &&
        (current_time - addr_timeouts[0].last_receive_time) > timeout_ms)
    {
        // 重置地址1对应的变量
        lcd_show_addr_1 = 0x00;
        lcd_show_pitch_1 = 0.00f;
        lcd_show_roll_1 = 0.00f;
        lcd_show_range_1 = 0;
        addr_timeouts[0].last_receive_time = 0; // 标记为无数据
    }

    // 检查地址2 (0x2026) 是否超时
    if (addr_timeouts[1].last_receive_time > 0 &&
        (current_time - addr_timeouts[1].last_receive_time) > timeout_ms)
    {
        // 重置地址2对应的变量
        lcd_show_addr_2 = 0x00;
        lcd_show_pitch_2 = 0.00f;
        lcd_show_roll_2 = 0.00f;
        lcd_show_range_2 = 0;
        addr_timeouts[1].last_receive_time = 0; // 标记为无数据
    }
}

/**
 * @brief UART接收中断回调函数（协议包解析核心）
 * @param hdl 数据链路层句柄指针
 * @note 在HAL_UART_RxCpltCallback中调用
 */
void DL_UART_RxCpltCallback(DL_HandleTypeDef *hdl)
{
    if (hdl->rx_buffer[hdl->rx_index] == DL_PACKET_FOOTER)
    {
        uint8_t *raw_data = hdl->rx_buffer;
        uint16_t total_len = hdl->rx_index + 1;
        
        // 协议验证
        if (raw_data[0] == DL_PACKET_HEADER &&
            raw_data[total_len - 1] == DL_PACKET_FOOTER &&
            total_len >= 15) // 9字节数据 + 6字节包头 = 15字节（FE 0F ... FF）
        {
            // 解析协议字段
            uint8_t data_len = raw_data[1] - 4; // 减去协议头长度
            uint16_t remote_addr = raw_data[4] | (raw_data[5] << 8);

            // 提取传感器数据
            float pitch, roll;
            uint8_t range;

            if (data_len == 9)
            {
                memcpy(&pitch, &raw_data[6], 4);
                memcpy(&roll, &raw_data[10], 4);
                range = raw_data[14];

                // 合法性校验
                bool valid = true;

                // 1. 检查地址是否为特殊值2025或2026
                char addr_str[5];
                snprintf(addr_str, sizeof(addr_str), "%04x", remote_addr);
                
                // 2. 检查pitch和roll是否为NaN、无穷且绝对值合理
                if (isnan(pitch) || isinf(pitch) || fabsf(pitch) > 360.00f)
                {
                    valid = false;
                }
                if (isnan(roll) || isinf(roll) || fabsf(roll) > 360.00f)
                {
                    valid = false;
                }

                // 3. 检查极端值情况（如-0.00或极大值）
                if ((fabsf(pitch) < 0.001f && pitch != 0.0f) || 
                    (fabsf(roll) < 0.001f && roll != 0.0f))
                {
                    valid = false; // 过滤-0.00等异常值
                }
                
                // 4. 检查range是否在合理范围内
                if (range == 0 || range > 255)
                {
                    valid = false;
                }

                // 5. 检查特殊地址2025的数据是否合理
                if (strcmp(addr_str, "2025") == 0)
                {
                    // 检查是否为极大值（如-2.68e59）
                    if (fabsf(pitch) > 1e10f || fabsf(roll) > 1e10f)
                    {
                        valid = false;
                    }
                }

                if (valid)
                {
                    lcd_show_addr = remote_addr;
                    sprintf(lcd_show_addr_buffer, "%04x", lcd_show_addr);

                    // 更新对应地址的最后接收时间
                    if (strcmp(lcd_show_addr_buffer, "2025") == 0)
                    {
                        addr_timeouts[0].last_receive_time = HAL_GetTick();
                        lcd_show_addr_1 = remote_addr;
                        lcd_show_pitch_1 = pitch;
                        lcd_show_roll_1 = roll;
                        lcd_show_range_1 = range;
                    }
                    else if (strcmp(lcd_show_addr_buffer, "2026") == 0)
                    {
                        addr_timeouts[1].last_receive_time = HAL_GetTick();
                        lcd_show_addr_2 = remote_addr;
                        lcd_show_pitch_2 = pitch;
                        lcd_show_roll_2 = roll;
                        lcd_show_range_2 = range;
                    }

                    // 转换为字符串
                    char buffer[64];
                    int str_len = snprintf(buffer, sizeof(buffer),
                                           "%04X,%.2f,%.2f,%d\n",
                                           remote_addr, pitch, roll, range);

                    // 输出
                    printf("%s", buffer);
                }
                else
                {
                    printf("Invalid data detected: %04X,%.2f,%.2f,%d\n", 
                           remote_addr, pitch, roll, range);
                }
            }
        }
        hdl->rx_index = 0;
    }
    else
    {
        hdl->rx_index = (hdl->rx_index + 1) % sizeof(hdl->rx_buffer);
    }
    HAL_UART_Receive_IT(hdl->huart, &hdl->rx_buffer[hdl->rx_index], 1);
}


uint16_t DL_LN3X_Get_Addr_1(void)
{
    return lcd_show_addr_1;
}

uint16_t DL_LN3X_Get_Addr_2(void)
{
    return lcd_show_addr_2;
}

float DL_LN3X_Get_Pitch_1(void)
{
    return lcd_show_pitch_1;
}

float DL_LN3X_Get_Roll_1(void)
{
    return lcd_show_roll_1;
}

uint8_t DL_LN3X_Get_Range_1(void)
{
    return lcd_show_range_1;
}

float DL_LN3X_Get_Pitch_2(void)
{
    return lcd_show_pitch_2;
}

float DL_LN3X_Get_Roll_2(void)
{
    return lcd_show_roll_2;
}

uint8_t DL_LN3X_Get_Range_2(void)
{
    return lcd_show_range_2;
}