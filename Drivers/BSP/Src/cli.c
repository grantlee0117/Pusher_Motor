#include "cli.h"
#include "pusher_motor.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// CLI 配置实例
CliConfig_t cli_config;

// RX DMA 句柄（由 CubeMX 生成）
extern DMA_HandleTypeDef hdma_usart1_rx;

// ==========================================================
//  RX DMA 接收
// ==========================================================
#define RX_DMA_BUF_SIZE 256
uint8_t rx_dma_buffer[RX_DMA_BUF_SIZE];

// 接收环形缓冲区（DMA IDLE 中断写入，主循环读取）
#define RX_BUF_SIZE 512
static uint8_t rx_ring_buf[RX_BUF_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

// 命令行缓冲区
static uint8_t cmd_buf[RX_BUF_SIZE];
static uint16_t cmd_len = 0;

/**
 * @brief 将字符写入接收环形缓冲区（中断安全）
 */
static void rx_buf_write(uint8_t data)
{
    uint16_t next_head = (rx_head + 1) % RX_BUF_SIZE;
    if (next_head != rx_tail)
    {
        rx_ring_buf[rx_head] = data;
        rx_head = next_head;
    }
}

/**
 * @brief 从接收环形缓冲区读取字符
 * @return 0: 无数据, 1: 有数据
 */
static int rx_buf_read(uint8_t *data)
{
    if (rx_head == rx_tail)
    {
        return 0;
    }
    *data = rx_ring_buf[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
    return 1;
}

static uint16_t rx_dma_last_pos = 0;

/**
 * @brief RX DMA 数据处理（在 USART IDLE 中断中调用）
 * @note 使用 last_pos 记录上次处理位置，只处理新数据，避免重复处理历史数据
 */
void cli_process_rx_dma(uint8_t *data, uint16_t len)
{
    (void)len; /* len 参数由 DMA 计数器计算，不使用传入值 */

    uint16_t curr_pos = RX_DMA_BUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);

    /* 计算新数据长度 */
    uint16_t rx_len;
    if (curr_pos >= rx_dma_last_pos)
    {
        rx_len = curr_pos - rx_dma_last_pos;
    }
    else
    {
        /* DMA 绕圈了 */
        rx_len = RX_DMA_BUF_SIZE - rx_dma_last_pos + curr_pos;
    }

    /* 处理新数据 */
    for (uint16_t i = 0; i < rx_len; i++)
    {
        uint16_t idx = (rx_dma_last_pos + i) % RX_DMA_BUF_SIZE;
        rx_buf_write(data[idx]);
    }

    rx_dma_last_pos = curr_pos;
}

// ==========================================================
//  命令解析
// ==========================================================

/**
 * @brief 去除字符串首尾空白字符
 */
static void trim_string(char *str)
{
    if (str == NULL)
        return;

    int len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1]))
    {
        str[--len] = '\0';
    }

    int start = 0;
    while (str[start] && isspace((unsigned char)str[start]))
    {
        start++;
    }

    if (start > 0)
    {
        memmove(str, str + start, len - start + 1);
    }
}

/**
 * @brief 解析 device motor push=time,wait,duty,accel 命令
 * @param cmd_str 输入命令字符串
 * @param direction_time_ms 输出：运行时间（ms）
 * @param wait_time_ms 输出：等待时间（ms）
 * @param pwm_duty 输出：PWM占空比（5-95）
 * @param acceleration 输出：加速度步长（0-50），0=无加速
 * @return 0: 解析成功, -1: 解析失败
 */
static int parse_device_motor_push(char *cmd_str,
                                    uint32_t *direction_time_ms,
                                    uint32_t *wait_time_ms,
                                    uint32_t *pwm_duty,
                                    uint32_t *acceleration)
{
    trim_string(cmd_str);

    if (strlen(cmd_str) == 0)
    {
        return -1;
    }

    /* 检查前缀 "device motor push=" */
    const char *prefix = "device motor push=";
    if (strncasecmp(cmd_str, prefix, strlen(prefix)) != 0)
    {
        return -1;
    }

    /* 获取等号后面的参数部分 */
    char *params = cmd_str + strlen(prefix);
    if (strlen(params) == 0)
    {
        return -1;
    }

    /* 解析四个参数：time,wait,duty,accel */
    char *token;
    char *saveptr;

    /* 第一个参数：运行时间 */
    token = strtok_r(params, ",", &saveptr);
    if (token == NULL)
        return -1;
    *direction_time_ms = (uint32_t)atoi(token);

    /* 第二个参数：等待时间 */
    token = strtok_r(NULL, ",", &saveptr);
    if (token == NULL)
        return -1;
    *wait_time_ms = (uint32_t)atoi(token);

    /* 第三个参数：PWM占空比（速度百分比） */
    token = strtok_r(NULL, ",", &saveptr);
    if (token == NULL)
        return -1;
    *pwm_duty = (uint32_t)atoi(token);

    /* 第四个参数：加速度步长（可选，默认0） */
    token = strtok_r(NULL, ",", &saveptr);
    if (token == NULL)
    {
        *acceleration = 0;
    }
    else
    {
        *acceleration = (uint32_t)atoi(token);
    }

    /* 参数范围校验 */
    if (*direction_time_ms < 1 || *direction_time_ms > 9999)
        return -1;
    if (*wait_time_ms > 9999)
        return -1;
    if (*pwm_duty < 5 || *pwm_duty > 100)
        return -1;
    if (*acceleration > 50)
        return -1;

    /* 占空比上限限制：大于95强制设为95 */
    if (*pwm_duty > 95)
    {
        *pwm_duty = 95;
    }

    return 0;
}

/**
 * @brief 执行命令
 */
static void cli_execute_command(CliCommand_t cmd,
                                 uint32_t direction_time_ms,
                                 uint32_t wait_time_ms,
                                 uint32_t pwm_duty,
                                 uint32_t acceleration)
{
    switch (cmd)
    {
    case CLI_CMD_DEVICE_MOTOR_PUSH:
        /* 设置加速度 */
        pusher_motor_set_acceleration((uint8_t)acceleration);
        /* 设置参数并启动电机 */
        pusher_motor_set_params_and_start(direction_time_ms, wait_time_ms, pwm_duty);
        break;

    case CLI_CMD_UNKNOWN:
    default:
        /* 未知命令：静默忽略，不做任何响应 */
        break;
    }
}

// ==========================================================
//  CLI 核心逻辑
// ==========================================================

/**
 * @brief 处理命令行输入
 * @return 1: 命令就绪, 0: 继续接收
 */
static int process_input_line(uint8_t data)
{
    if (data == '\r' || data == '\n')
    {
        if (cmd_len > 0)
        {
            cmd_buf[cmd_len] = '\0';
            return 1;
        }
        return 0;
    }

    /* 忽略退格和其他控制字符 */
    if (data < 0x20 || data >= 0x7F)
    {
        return 0;
    }

    if (cmd_len < RX_BUF_SIZE - 1)
    {
        cmd_buf[cmd_len++] = data;
        cmd_buf[cmd_len] = '\0';
    }

    return 0;
}

// ==========================================================
//  CLI 初始化和主循环
// ==========================================================

/**
 * @brief CLI 初始化函数
 */
void cli_init(UART_HandleTypeDef *huart)
{
    cli_config.huart = huart;

    // 清空接收环形缓冲区
    rx_head = 0;
    rx_tail = 0;
    memset(rx_ring_buf, 0, sizeof(rx_ring_buf));

    // 清空命令缓冲区
    cmd_len = 0;
    memset(cmd_buf, 0, sizeof(cmd_buf));

    /* 启动 RX DMA（CIRCULAR 模式） */
    HAL_UART_Receive_DMA(huart, rx_dma_buffer, RX_DMA_BUF_SIZE);

    /* 使能 UART IDLE 中断 */
    __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
}

/**
 * @brief CLI 处理函数（主循环调用）
 */
void cli_process(void)
{
    uint8_t data;
    while (rx_buf_read(&data))
    {
        if (process_input_line(data))
        {
            uint32_t direction_time_ms = 0;
            uint32_t wait_time_ms = 0;
            uint32_t pwm_duty = 0;
            uint32_t acceleration = 0;

            char cmd_str[RX_BUF_SIZE];
            strncpy(cmd_str, (char *)cmd_buf, sizeof(cmd_str) - 1);
            cmd_str[sizeof(cmd_str) - 1] = '\0';

            if (parse_device_motor_push(cmd_str, &direction_time_ms, &wait_time_ms, &pwm_duty, &acceleration) == 0)
            {
                cli_execute_command(CLI_CMD_DEVICE_MOTOR_PUSH,
                                    direction_time_ms, wait_time_ms, pwm_duty, acceleration);
            }
            else
            {
                /* 解析失败：静默忽略，不做任何响应 */
            }

            cmd_len = 0;
            memset(cmd_buf, 0, sizeof(cmd_buf));

            break;
        }
    }
}
