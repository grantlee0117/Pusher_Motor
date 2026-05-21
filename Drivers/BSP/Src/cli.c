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
//  TX DMA 发送队列
// ==========================================================
#define TX_QUEUE_SIZE 8
#define TX_NODE_SIZE 128

typedef struct
{
    char buf[TX_NODE_SIZE];
    uint16_t len;
    volatile uint8_t ready; /* 0:空闲, 1:待发送 */
} TxNode_t;

static TxNode_t tx_queue[TX_QUEUE_SIZE];
static volatile uint8_t tx_queue_head = 0; /* 写入位置 */
static volatile uint8_t tx_queue_tail = 0; /* 发送位置 */
static volatile uint8_t tx_busy = 0;

/**
 * @brief 获取队列下一个索引
 */
static uint8_t tx_queue_next(uint8_t idx)
{
    return (idx + 1) % TX_QUEUE_SIZE;
}

/**
 * @brief 检查队列是否满
 */
static int tx_queue_full(void)
{
    return tx_queue_next(tx_queue_head) == tx_queue_tail;
}

/**
 * @brief 检查队列是否空
 */
static int tx_queue_empty(void)
{
    return tx_queue_head == tx_queue_tail;
}

/**
 * @brief 将字符串加入发送队列
 * @return 0:成功, 1:队列满
 */
static int tx_queue_put(const char *str, uint16_t len)
{
    if (tx_queue_full())
    {
        return 1; /* 队列满 */
    }

    TxNode_t *node = &tx_queue[tx_queue_head];
    if (len >= TX_NODE_SIZE)
    {
        len = TX_NODE_SIZE - 1;
    }

    memcpy(node->buf, str, len);
    node->buf[len] = '\0';
    node->len = len;
    node->ready = 1;

    tx_queue_head = tx_queue_next(tx_queue_head);
    return 0;
}

/**
 * @brief 启动 DMA 发送队列中的下一帧
 */
static void tx_queue_start_dma(void)
{
    if (tx_busy || tx_queue_empty())
    {
        return;
    }

    TxNode_t *node = &tx_queue[tx_queue_tail];
    if (!node->ready)
    {
        return;
    }

    tx_busy = 1;
    HAL_UART_Transmit_DMA(cli_config.huart, (uint8_t *)node->buf, node->len);
}

/**
 * @brief DMA 发送完成回调（在 HAL_UART_TxCpltCallback 中调用）
 */
void cli_tx_dma_complete(void)
{
    if (!tx_busy)
    {
        return;
    }

    /* 释放当前节点 */
    TxNode_t *node = &tx_queue[tx_queue_tail];
    node->ready = 0;
    node->len = 0;

    tx_queue_tail = tx_queue_next(tx_queue_tail);
    tx_busy = 0;

    /* 启动下一帧 */
    tx_queue_start_dma();
}

// ==========================================================
//  公共发送接口
// ==========================================================

/**
 * @brief 发送字符串到串口（DMA 队列方式）
 */
void cli_send_string(const char *str)
{
    if (str == NULL || cli_config.huart == NULL)
        return;

    uint16_t len = strlen(str);
    if (len == 0)
        return;

    /* 等待队列空闲 */
    uint32_t timeout = HAL_GetTick() + 50;
    while (tx_queue_full())
    {
        if (HAL_GetTick() >= timeout)
            return;
    }

    tx_queue_put(str, len);
    tx_queue_start_dma();
}

/**
 * @brief 发送字符串（内部使用，与 cli_send_string 相同）
 */
static void cli_puts(const char *str)
{
    cli_send_string(str);
}

/**
 * @brief 发送单个字符
 */
// ==========================================================
//  CLI 核心逻辑
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
 *
 */
static int parse_command(char *cmd_str, CliCommand_t *cmd, char **param)
{
    *cmd = CLI_CMD_UNKNOWN;
    *param = NULL;

    trim_string(cmd_str);

    if (strlen(cmd_str) == 0)
    {
        return -1;
    }

    char *space = strchr(cmd_str, ' ');
    if (space)
    {
        *space = '\0';
        *param = space + 1;
        trim_string(*param);
    }

    if (strcmp(cmd_str, "start") == 0)
    {
        *cmd = CLI_CMD_START;
    }
    else if (strcmp(cmd_str, "help") == 0 || strcmp(cmd_str, "?") == 0)
    {
        *cmd = CLI_CMD_HELP;
    }
    else if (strcmp(cmd_str, "get") == 0)
    {
        if (*param == NULL)
            return -1;
        if (strcmp(*param, "direction_time") == 0)
        {
            *cmd = CLI_CMD_GET_DIRECTION_TIME;
            *param = NULL;
        }
        else if (strcmp(*param, "pwm_duty") == 0)
        {
            *cmd = CLI_CMD_GET_PWM_DUTY;
            *param = NULL;
        }
        else if (strcmp(*param, "wait_time") == 0)
        {
            *cmd = CLI_CMD_GET_WAIT_TIME;
            *param = NULL;
        }
        else if (strcmp(*param, "max_speed") == 0)
        {
            *cmd = CLI_CMD_GET_MAX_SPEED;
            *param = NULL;
        }
        else if (strcmp(*param, "speed") == 0)
        {
            *cmd = CLI_CMD_GET_SPEED;
            *param = NULL;
        }
        else if (strcmp(*param, "motor_mp_a_dir") == 0)
        {
            *cmd = CLI_CMD_GET_MOTOR_MP_A_DIR;
            *param = NULL;
        }
        else if (strcmp(*param, "motor_mp_b_dir") == 0)
        {
            *cmd = CLI_CMD_GET_MOTOR_MP_B_DIR;
            *param = NULL;
        }
        else if (strcmp(*param, "acceleration") == 0)
        {
            *cmd = CLI_CMD_GET_ACCELERATION;
            *param = NULL;
        }
        else if (strcmp(*param, "start_signal") == 0)
        {
            *cmd = CLI_CMD_GET_START_SIGNAL;
            *param = NULL;
        }
    }
    else if (strcmp(cmd_str, "set") == 0)
    {
        if (*param == NULL)
            return -1;

        char *subcmd = *param;
        char *value = strchr(subcmd, ' ');

        if (value)
        {
            *value = '\0';
            value++;
            trim_string(value);

            if (strcmp(subcmd, "direction_time") == 0)
            {
                *cmd = CLI_CMD_SET_DIRECTION_TIME;
                *param = value;
            }
            else if (strcmp(subcmd, "pwm_duty") == 0)
            {
                *cmd = CLI_CMD_SET_PWM_DUTY;
                *param = value;
            }
            else if (strcmp(subcmd, "wait_time") == 0)
            {
                *cmd = CLI_CMD_SET_WAIT_TIME;
                *param = value;
            }
            else if (strcmp(subcmd, "max_speed") == 0)
            {
                *cmd = CLI_CMD_SET_MAX_SPEED;
                *param = value;
            }
            else if (strcmp(subcmd, "speed") == 0)
            {
                *cmd = CLI_CMD_SET_SPEED;
                *param = value;
            }
            else if (strcmp(subcmd, "motor_mp_a_dir") == 0)
            {
                *cmd = CLI_CMD_SET_MOTOR_MP_A_DIR;
                *param = value;
            }
            else if (strcmp(subcmd, "motor_mp_b_dir") == 0)
            {
                *cmd = CLI_CMD_SET_MOTOR_MP_B_DIR;
                *param = value;
            }
            else if (strcmp(subcmd, "new_pwm_duty") == 0)
            {
                if (value != NULL && strlen(value) > 0)
                {
                    int32_t duty_val = atoi(value);
                    if (duty_val >= 0 && duty_val <= 100)
                    {
                        pusher_motor_set_pwm_duty_direct((uint32_t)duty_val);
                        cli_send_string("PWM duty directly set to ");
                        char duty_str[16];
                        snprintf(duty_str, sizeof(duty_str), "%ld", (long)duty_val);
                        cli_send_string(duty_str);
                        cli_send_string(" (not saved to Flash)\r\n> ");
                    }
                    else
                    {
                        cli_send_string("Error: Invalid value. Range: 0-100\r\n> ");
                    }
                }
                else
                {
                    cli_send_string("Error: Missing value. Usage: set new_pwm_duty <0-100>\r\n> ");
                }
                return -2;
            }
            else if (strcmp(subcmd, "acceleration") == 0)
            {
                *cmd = CLI_CMD_SET_ACCELERATION;
                *param = value;
            }
        }
    }

    return (*cmd == CLI_CMD_UNKNOWN) ? -1 : 0;
}

/**
 * @brief 执行命令
 */
void cli_execute_command(CliCommand_t cmd, const char *param)
{
    char response[128];
    int32_t value;

    switch (cmd)
    {
    case CLI_CMD_START:
        pusher_motor_start();
        cli_send_string("Pusher motor started.\r\n");
        break;

    case CLI_CMD_SET_DIRECTION_TIME:
        if (param == NULL || strlen(param) == 0)
        {
            cli_send_string("Error: Missing value. Usage: set direction_time <ms>\r\n");
            break;
        }
        value = atoi(param);
        if (value > 0 && value <= 60000)
        {
            if (pusher_motor_set_direction_time((uint32_t)value) == 0)
            {
                snprintf(response, sizeof(response), "Direction time set to %lu ms (saved to Flash)\r\n", (unsigned long)value);
            }
            else
            {
                snprintf(response, sizeof(response), "Direction time set to %lu ms (Flash save failed)\r\n", (unsigned long)value);
            }
            cli_send_string(response);
        }
        else
        {
            cli_send_string("Error: Invalid value. Range: 1-60000 ms\r\n");
        }
        break;

    case CLI_CMD_SET_PWM_DUTY:
        if (param == NULL || strlen(param) == 0)
        {
            cli_send_string("Error: Missing value. Usage: set pwm_duty <0-100>\r\n");
            break;
        }
        value = atoi(param);
        if (value >= 0 && value <= 100)
        {
            if (pusher_motor_set_pwm_duty((uint32_t)value) == 0)
            {
                snprintf(response, sizeof(response), "PWM duty set to %lu (saved to Flash)\r\n", (unsigned long)value);
            }
            else
            {
                snprintf(response, sizeof(response), "PWM duty set to %lu (Flash save failed)\r\n", (unsigned long)value);
            }
            cli_send_string(response);
        }
        else
        {
            cli_send_string("Error: Invalid value. Range: 0-100\r\n");
        }
        break;

    case CLI_CMD_GET_DIRECTION_TIME:
        snprintf(response, sizeof(response), "Current direction time: %lu ms\r\n", (unsigned long)pusher_motor_get_direction_time());
        cli_send_string(response);
        break;

    case CLI_CMD_GET_PWM_DUTY:
        snprintf(response, sizeof(response), "Current PWM duty: %lu\r\n", (unsigned long)pusher_motor_get_pwm_duty());
        cli_send_string(response);
        break;

    case CLI_CMD_GET_WAIT_TIME:
        snprintf(response, sizeof(response), "Current wait time: %lu ms\r\n", (unsigned long)pusher_motor_get_wait_time());
        cli_send_string(response);
        break;

    case CLI_CMD_GET_MAX_SPEED:
        snprintf(response, sizeof(response), "Current max speed: %lu RPM\r\n", (unsigned long)pusher_motor_get_max_speed());
        cli_send_string(response);
        break;

    case CLI_CMD_GET_SPEED:
    {
        uint32_t speed = pusher_motor_calculate_speed_from_duty();
        snprintf(response, sizeof(response), "Current speed: %lu cm/min\r\n", (unsigned long)speed);
        cli_send_string(response);
    }
    break;

    case CLI_CMD_GET_MOTOR_MP_A_DIR:
        snprintf(response, sizeof(response), "Current motor MP A direction: %lu\r\n", (unsigned long)pusher_motor_get_motor_mp_a_dir());
        cli_send_string(response);
        break;

    case CLI_CMD_GET_MOTOR_MP_B_DIR:
        snprintf(response, sizeof(response), "Current motor MP B direction: %lu\r\n", (unsigned long)pusher_motor_get_motor_mp_b_dir());
        cli_send_string(response);
        break;

    case CLI_CMD_SET_MAX_SPEED:
        if (param == NULL || strlen(param) == 0)
        {
            cli_send_string("Error: Missing value. Usage: set max_speed <rpm>\r\n");
            break;
        }
        value = atoi(param);
        if (value > 0 && value <= 10000)
        {
            if (pusher_motor_set_max_speed((uint32_t)value) == 0)
            {
                snprintf(response, sizeof(response), "Max speed set to %lu RPM (saved to Flash)\r\n", (unsigned long)value);
            }
            else
            {
                snprintf(response, sizeof(response), "Max speed set to %lu RPM (Flash save failed)\r\n", (unsigned long)value);
            }
            cli_send_string(response);
        }
        else
        {
            cli_send_string("Error: Invalid value. Range: 1-10000 RPM\r\n");
        }
        break;

    case CLI_CMD_SET_SPEED:
        if (param == NULL || strlen(param) == 0)
        {
            float circumference = 3.14159f * 6.0f;
            uint32_t max_speed = (uint32_t)((float)pusher_motor_get_max_speed() * circumference);
            snprintf(response, sizeof(response), "Error: Missing value. Usage: set speed <cm/min> (Range: 0-%lu cm/min)\r\n", (unsigned long)max_speed);
            cli_send_string(response);
            break;
        }
        value = atoi(param);
        if (value >= 0)
        {
            if (pusher_motor_set_speed((uint32_t)value) == 0)
            {
                uint32_t duty = pusher_motor_calculate_duty_from_speed((uint32_t)value);
                float circumference = 3.14159f * 6.0f;
                uint32_t max_speed = (uint32_t)((float)pusher_motor_get_max_speed() * circumference);
                snprintf(response, sizeof(response), "Speed set to %lu cm/min (PWM duty: %lu) (Range: 0-%lu cm/min) (saved to Flash)\r\n",
                         (unsigned long)value, (unsigned long)duty, (unsigned long)max_speed);
            }
            else
            {
                snprintf(response, sizeof(response), "Speed set to %lu cm/min (Flash save failed)\r\n", (unsigned long)value);
            }
            cli_send_string(response);
        }
        else
        {
            float circumference = 3.14159f * 6.0f;
            uint32_t max_speed = (uint32_t)((float)pusher_motor_get_max_speed() * circumference);
            snprintf(response, sizeof(response), "Error: Invalid value. Must be non-negative (Range: 0-%lu cm/min)\r\n", (unsigned long)max_speed);
            cli_send_string(response);
        }
        break;

    case CLI_CMD_SET_WAIT_TIME:
        if (param == NULL || strlen(param) == 0)
        {
            cli_send_string("Error: Missing value. Usage: set wait_time <ms>\r\n");
            break;
        }
        value = atoi(param);
        if (value >= 0 && value <= 10000)
        {
            if (pusher_motor_set_wait_time((uint32_t)value) == 0)
            {
                snprintf(response, sizeof(response), "Wait time set to %lu ms (saved to Flash)\r\n", (unsigned long)value);
            }
            else
            {
                snprintf(response, sizeof(response), "Wait time set to %lu ms (Flash save failed)\r\n", (unsigned long)value);
            }
            cli_send_string(response);
        }
        else
        {
            cli_send_string("Error: Invalid value. Range: 0-10000 ms\r\n");
        }
        break;

    case CLI_CMD_SET_MOTOR_MP_A_DIR:
        if (param == NULL || strlen(param) == 0)
        {
            cli_send_string("Error: Missing value. Usage: set motor_mp_a_dir <0|1>\r\n");
            break;
        }
        value = atoi(param);
        if (value == 0 || value == 1)
        {
            if (pusher_motor_set_motor_mp_a_dir((uint32_t)value) == 0)
            {
                snprintf(response, sizeof(response), "Motor MP A direction set to %lu (saved to Flash)\r\n", (unsigned long)value);
            }
            else
            {
                snprintf(response, sizeof(response), "Motor MP A direction set to %lu (Flash save failed)\r\n", (unsigned long)value);
            }
            cli_send_string(response);
        }
        else
        {
            cli_send_string("Error: Invalid value. Must be 0 or 1\r\n");
        }
        break;

    case CLI_CMD_SET_MOTOR_MP_B_DIR:
        if (param == NULL || strlen(param) == 0)
        {
            cli_send_string("Error: Missing value. Usage: set motor_mp_b_dir <0|1>\r\n");
            break;
        }
        value = atoi(param);
        if (value == 0 || value == 1)
        {
            if (pusher_motor_set_motor_mp_b_dir((uint32_t)value) == 0)
            {
                snprintf(response, sizeof(response), "Motor MP B direction set to %lu (saved to Flash)\r\n", (unsigned long)value);
            }
            else
            {
                snprintf(response, sizeof(response), "Motor MP B direction set to %lu (Flash save failed)\r\n", (unsigned long)value);
            }
            cli_send_string(response);
        }
        else
        {
            cli_send_string("Error: Invalid value. Must be 0 or 1\r\n");
        }
        break;

    case CLI_CMD_SET_ACCELERATION:
        if (param == NULL || strlen(param) == 0)
        {
            cli_send_string("Error: Missing value. Usage: set acceleration <0-50>\r\n");
            break;
        }
        value = atoi(param);
        if (value >= 0 && value <= 50)
        {
            if (pusher_motor_set_acceleration((uint8_t)value) == 0)
            {
                snprintf(response, sizeof(response), "Acceleration set to %lu (0=off)\r\n", (unsigned long)value);
            }
            else
            {
                snprintf(response, sizeof(response), "Acceleration set to %lu failed\r\n", (unsigned long)value);
            }
            cli_send_string(response);
        }
        else
        {
            cli_send_string("Error: Invalid value. Range: 0-50\r\n");
        }
        break;

    case CLI_CMD_GET_ACCELERATION:
        snprintf(response, sizeof(response), "Current acceleration: %u (0=off, higher=faster ramp)\r\n", (unsigned int)pusher_motor_get_acceleration());
        cli_send_string(response);
        break;

    case CLI_CMD_GET_START_SIGNAL:
    {
        uint8_t level = pusher_motor_get_start_signal();
        snprintf(response, sizeof(response), "Current MOTOR_PM_ENABLE pin level: %u (%s)\r\n", (unsigned int)level, level ? "HIGH" : "LOW");
        cli_send_string(response);
    }
    break;

    case CLI_CMD_HELP:
        cli_send_string("\r\nAvailable commands:\r\n");
        cli_send_string("  start                    - Start pusher motor\r\n");
        cli_send_string("  set direction_time <ms>  - Set direction time (1-60000 ms)\r\n");
        cli_send_string("  set pwm_duty <value>     - Set PWM duty cycle (0-100)\r\n");
        cli_send_string("  set wait_time <ms>       - Set wait time (0-10000 ms)\r\n");
        cli_send_string("  set max_speed <rpm>      - Set max speed (1-10000 RPM)\r\n");
        cli_send_string("  set speed <cm/min>       - Set speed (0+ cm/min)\r\n");
        cli_send_string("  set motor_mp_a_dir <0|1> - Set motor MP A direction\r\n");
        cli_send_string("  set motor_mp_b_dir <0|1> - Set motor MP B direction\r\n");
        cli_send_string("  set new_pwm_duty <value> - Directly set PWM duty cycle (0-100, not saved)\r\n");
        cli_send_string("  set acceleration <0-50>  - Set acceleration step (0=off)\r\n");
        cli_send_string("  get direction_time       - Get current direction time\r\n");
        cli_send_string("  get pwm_duty             - Get current PWM duty cycle\r\n");
        cli_send_string("  get wait_time            - Get current wait time\r\n");
        cli_send_string("  get max_speed            - Get current max speed\r\n");
        cli_send_string("  get speed                - Get current speed\r\n");
        cli_send_string("  get motor_mp_a_dir       - Get current motor MP A direction\r\n");
        cli_send_string("  get motor_mp_b_dir       - Get current motor MP B direction\r\n");
        cli_send_string("  get acceleration         - Get current acceleration step\r\n");
        cli_send_string("  get start_signal         - Get MOTOR_PM_ENABLE pin level (PA0)\r\n");
        cli_send_string("  help / ?                 - Show this help\r\n");
        cli_send_string("\r\n");
        break;

    case CLI_CMD_UNKNOWN:
    default:
        cli_send_string("Unknown command. Type 'help' for available commands.\r\n");
        break;
    }
}

// ==========================================================
//  回显处理
// ==========================================================

/**
 * @brief 回显字符
 */
static void cli_echo(uint8_t data)
{
    (void)data;
    /* 回显已关闭：响应中不包含发送的指令 */
}

/**
 * @brief 处理退格键
 */
static void handle_backspace(void)
{
    if (cmd_len > 0)
    {
        cmd_len--;
        cmd_buf[cmd_len] = '\0';
        cli_echo('\b');
    }
}

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

    if (data == '\b' || data == 0x7F)
    {
        handle_backspace();
        return 0;
    }

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
    cli_config.state = CLI_STATE_IDLE;

    // 清空接收环形缓冲区
    rx_head = 0;
    rx_tail = 0;
    memset(rx_ring_buf, 0, sizeof(rx_ring_buf));

    // 清空发送队列
    tx_queue_head = 0;
    tx_queue_tail = 0;
    tx_busy = 0;
    for (int i = 0; i < TX_QUEUE_SIZE; i++)
    {
        tx_queue[i].ready = 0;
        tx_queue[i].len = 0;
    }

    // 清空命令缓冲区
    cmd_len = 0;
    memset(cmd_buf, 0, sizeof(cmd_buf));

    /* 启动 RX DMA（CIRCULAR 模式） */
    HAL_UART_Receive_DMA(huart, rx_dma_buffer, RX_DMA_BUF_SIZE);

    /* 使能 UART IDLE 中断 */
    __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);

    /* 欢迎信息 */
    cli_send_string("\r\n================================\r\n");
    cli_send_string("  CLI initialized.\r\n");
    cli_send_string("  Type 'help' for commands.\r\n");
    cli_send_string("================================\r\n");
    cli_send_string("> ");
}

/**
 * @brief HAL UART TX DMA 完成回调
 * @note 在 DMA 发送完成时由 HAL 库调用
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        cli_tx_dma_complete();
    }
}

/**
 * @brief CLI 处理函数（主循环调用）
 */
void cli_process(void)
{
    /* RX 数据由 DMA + IDLE 中断自动写入环形缓冲区 */

    uint8_t data;
    while (rx_buf_read(&data))
    {
        if (process_input_line(data))
        {
            CliCommand_t cmd;
            char *param = NULL;

            char cmd_str[RX_BUF_SIZE];
            strncpy(cmd_str, (char *)cmd_buf, sizeof(cmd_str) - 1);
            cmd_str[sizeof(cmd_str) - 1] = '\0';

            int parse_result = parse_command(cmd_str, &cmd, &param);
            if (parse_result == 0)
            {
                cli_execute_command(cmd, param);
            }
            else if (parse_result == -2)
            {
                /* 命令已在解析时直接处理 */
            }
            else
            {
                cli_execute_command(CLI_CMD_UNKNOWN, NULL);
            }

            cmd_len = 0;
            memset(cmd_buf, 0, sizeof(cmd_buf));

            if (parse_result != -2)
            {
                cli_puts("> ");
            }

            break;
        }
    }
}
