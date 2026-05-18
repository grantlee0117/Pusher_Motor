#include "cli.h"
#include "pusher_motor.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// 直接访问PWM定时器（用于set new_pwm_duty立即执行）
extern TIM_HandleTypeDef htim4;
#define PUSHER_MOTOR_PWM_TIMER htim4
#define PUSHER_MOTOR_PWM_CHANNEL TIM_CHANNEL_4



// CLI 配置实例
CliConfig_t cli_config;

// 接收缓冲区 - 使用更大的缓冲区应对突发数据
#define RX_BUF_SIZE 512
static uint8_t rx_ring_buf[RX_BUF_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

// 发送缓冲区（用于回显）
#define TX_BUF_SIZE 256
static uint8_t tx_buf[TX_BUF_SIZE];
static volatile uint16_t tx_head = 0;
static volatile uint16_t tx_tail = 0;

// 命令行缓冲区
static uint8_t cmd_buf[RX_BUF_SIZE];
static uint16_t cmd_len = 0;

/**
 * @brief 将字符写入接收环形缓冲区（中断安全）
 */
static void rx_buf_write(uint8_t data)
{
    uint16_t next_head = (rx_head + 1) % RX_BUF_SIZE;
    if (next_head != rx_tail) {
        rx_ring_buf[rx_head] = data;
        rx_head = next_head;
    }
    // 如果缓冲区满，丢弃数据（可以在这里添加溢出标记）
}

/**
 * @brief 从接收环形缓冲区读取字符
 * @return 0: 无数据, 1: 有数据
 */
static int rx_buf_read(uint8_t *data)
{
    if (rx_head == rx_tail) {
        return 0;
    }
    *data = rx_ring_buf[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUF_SIZE;
    return 1;
}

/**
 * @brief 将字符写入发送缓冲区（用于回显）
 */
static void tx_buf_write(uint8_t data)
{
    uint16_t next_head = (tx_head + 1) % TX_BUF_SIZE;
    if (next_head != tx_tail) {
        tx_buf[tx_head] = data;
        tx_head = next_head;
    }
}

/**
 * @brief 从发送缓冲区读取字符
 * @return 0: 无数据, 1: 有数据
 */
static int tx_buf_read(uint8_t *data)
{
    if (tx_head == tx_tail) {
        return 0;
    }
    *data = tx_buf[tx_tail];
    tx_tail = (tx_tail + 1) % TX_BUF_SIZE;
    return 1;
}

/**
 * @brief 发送单个字符（非阻塞，放入发送缓冲区）
 */
static void cli_putchar(uint8_t c)
{
    tx_buf_write(c);
}

/**
 * @brief 发送字符串（非阻塞，放入发送缓冲区）
 */
static void cli_puts(const char *str)
{
    if (str == NULL) return;
    while (*str) {
        tx_buf_write(*str++);
    }
}

/**
 * @brief 处理发送（应在主循环中调用，或使用中断）
 */
static void cli_process_tx(void)
{
    if (cli_config.huart == NULL) return;
    
    // 检查发送寄存器是否空闲
    if (__HAL_UART_GET_FLAG(cli_config.huart, UART_FLAG_TXE) == RESET) {
        return;  // 发送寄存器忙，下次再试
    }
    
    uint8_t data;
    if (tx_buf_read(&data)) {
        cli_config.huart->Instance->DR = data;
    }
}

/**
 * @brief CLI 初始化函数
 * @param huart 串口句柄
 */
void cli_init(UART_HandleTypeDef *huart)
{
    cli_config.huart = huart;
    cli_config.state = CLI_STATE_IDLE;
    
    // 清空缓冲区
    rx_head = 0;
    rx_tail = 0;
    memset(rx_ring_buf, 0, sizeof(rx_ring_buf));
    
    tx_head = 0;
    tx_tail = 0;
    memset(tx_buf, 0, sizeof(tx_buf));
    
    cmd_len = 0;
    memset(cmd_buf, 0, sizeof(cmd_buf));

    // 发送欢迎信息（使用阻塞方式发送初始化信息）
    cli_send_string("\r\n================================\r\n");
    cli_send_string("  CLI initialized.\r\n");
    cli_send_string("  Type 'help' for commands.\r\n");
    cli_send_string("================================\r\n");
    cli_send_string("> ");
}

/**
 * @brief 发送字符串到串口（阻塞方式，用于响应输出）
 * @param str 要发送的字符串
 */
void cli_send_string(const char *str)
{
    if (str == NULL || cli_config.huart == NULL) return;
    HAL_UART_Transmit(cli_config.huart, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

/**
 * @brief 去除字符串首尾空白字符
 */
static void trim_string(char *str)
{
    if (str == NULL) return;
    
    // 去除尾部空白
    int len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
    
    // 去除头部空白
    int start = 0;
    while (str[start] && isspace((unsigned char)str[start])) {
        start++;
    }
    
    if (start > 0) {
        memmove(str, str + start, len - start + 1);
    }
}

/**
 * @brief 解析命令字符串
 * @param cmd_str 命令字符串
 * @param cmd 返回的命令类型
 * @param param 返回的参数指针
 * @return 0: 成功, -1: 失败
 */
static int parse_command(char *cmd_str, CliCommand_t *cmd, char **param)
{
    *cmd = CLI_CMD_UNKNOWN;
    *param = NULL;
    
    trim_string(cmd_str);
    
    if (strlen(cmd_str) == 0) {
        return -1;
    }
    
    // 查找第一个空格分隔命令和参数
    char *space = strchr(cmd_str, ' ');
    if (space) {
        *space = '\0';
        *param = space + 1;
        trim_string(*param);
    }
    
    // 解析命令
    if (strcmp(cmd_str, "start") == 0) {
        *cmd = CLI_CMD_START;
    }
    else if (strcmp(cmd_str, "help") == 0 || strcmp(cmd_str, "?") == 0) {
        *cmd = CLI_CMD_HELP;
    }
    else if (strcmp(cmd_str, "get") == 0) {
        if (*param == NULL) return -1;
        if (strcmp(*param, "direction_time") == 0) {
            *cmd = CLI_CMD_GET_DIRECTION_TIME;
            *param = NULL;
        }
        else if (strcmp(*param, "pwm_duty") == 0) {
            *cmd = CLI_CMD_GET_PWM_DUTY;
            *param = NULL;
        }
        else if (strcmp(*param, "wait_time") == 0) {
            *cmd = CLI_CMD_GET_WAIT_TIME;
            *param = NULL;
        }
        else if (strcmp(*param, "max_speed") == 0) {
            *cmd = CLI_CMD_GET_MAX_SPEED;
            *param = NULL;
        }
        else if (strcmp(*param, "speed") == 0) {
            *cmd = CLI_CMD_GET_SPEED;
            *param = NULL;
        }
        else if (strcmp(*param, "motor_mp_a_dir") == 0) {
            *cmd = CLI_CMD_GET_MOTOR_MP_A_DIR;
            *param = NULL;
        }
        else if (strcmp(*param, "motor_mp_b_dir") == 0) {
                *cmd = CLI_CMD_GET_MOTOR_MP_B_DIR;
                *param = NULL;
            }
            else if (strcmp(*param, "start_signal") == 0) {
                *cmd = CLI_CMD_GET_START_SIGNAL;
                *param = NULL;
            }
            else if (strcmp(*param, "acceleration") == 0) {
                *cmd = CLI_CMD_GET_ACCELERATION;
                *param = NULL;
            }
    }
    else if (strcmp(cmd_str, "set") == 0) {
        if (*param == NULL) return -1;
        
        // set 命令需要解析: set direction_time 1000 或 set pwm_duty 500
        char *subcmd = *param;
        char *value = strchr(subcmd, ' ');
        
        if (value) {
            *value = '\0';
            value++;
            trim_string(value);
            
            if (strcmp(subcmd, "direction_time") == 0) {
                *cmd = CLI_CMD_SET_DIRECTION_TIME;
                *param = value;
            }
            else if (strcmp(subcmd, "pwm_duty") == 0) {
                *cmd = CLI_CMD_SET_PWM_DUTY;
                *param = value;
            }
            else if (strcmp(subcmd, "wait_time") == 0) {
                *cmd = CLI_CMD_SET_WAIT_TIME;
                *param = value;
            }
            else if (strcmp(subcmd, "max_speed") == 0) {
                *cmd = CLI_CMD_SET_MAX_SPEED;
                *param = value;
            }
            else if (strcmp(subcmd, "speed") == 0) {
                *cmd = CLI_CMD_SET_SPEED;
                *param = value;
            }
            else if (strcmp(subcmd, "motor_mp_a_dir") == 0) {
                *cmd = CLI_CMD_SET_MOTOR_MP_A_DIR;
                *param = value;
            }
            else if (strcmp(subcmd, "motor_mp_b_dir") == 0) {
                *cmd = CLI_CMD_SET_MOTOR_MP_B_DIR;
                *param = value;
            }
            else if (strcmp(subcmd, "new_pwm_duty") == 0) {
                // 立即执行：直接设置PWM占空比，不经过状态机
                if (value != NULL && strlen(value) > 0) {
                    int32_t duty_val = atoi(value);
                    if (duty_val >= 0 && duty_val <= 500) {
                        __HAL_TIM_SET_COMPARE(&PUSHER_MOTOR_PWM_TIMER, PUSHER_MOTOR_PWM_CHANNEL, (uint32_t)duty_val);
                        cli_send_string("PWM duty directly set to ");
                        char duty_str[16];
                        snprintf(duty_str, sizeof(duty_str), "%ld", (long)duty_val);
                        cli_send_string(duty_str);
                        cli_send_string(" (not saved to Flash)\r\n> ");
                    } else {
                        cli_send_string("Error: Invalid value. Range: 0-500\r\n> ");
                    }
                } else {
                    cli_send_string("Error: Missing value. Usage: set new_pwm_duty <0-500>\r\n> ");
                }
                // 标记为已处理（返回-1表示已直接处理，无需状态机执行）
                return -2;
            }
            else if (strcmp(subcmd, "acceleration") == 0) {
                *cmd = CLI_CMD_SET_ACCELERATION;
                *param = value;
            }
        }
    }
    
    return (*cmd == CLI_CMD_UNKNOWN) ? -1 : 0;
}

/**
 * @brief 执行命令
 * @param cmd 命令类型
 * @param param 参数
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
            if (param == NULL || strlen(param) == 0) {
                cli_send_string("Error: Missing value. Usage: set direction_time <ms>\r\n");
                break;
            }
            value = atoi(param);
            if (value > 0 && value <= 60000) {
                if (pusher_motor_set_direction_time((uint32_t)value) == 0) {
                    snprintf(response, sizeof(response), "Direction time set to %lu ms (saved to Flash)\r\n", (unsigned long)value);
                } else {
                    snprintf(response, sizeof(response), "Direction time set to %lu ms (Flash save failed)\r\n", (unsigned long)value);
                }
                cli_send_string(response);
            }
            else {
                cli_send_string("Error: Invalid value. Range: 1-60000 ms\r\n");
            }
            break;

        case CLI_CMD_SET_PWM_DUTY:
            if (param == NULL || strlen(param) == 0) {
                cli_send_string("Error: Missing value. Usage: set pwm_duty <0-500>\r\n");
                break;
            }
            value = atoi(param);
            if (value >= 0 && value <= 500) {
                if (pusher_motor_set_pwm_duty((uint32_t)value) == 0) {
                    snprintf(response, sizeof(response), "PWM duty set to %lu (saved to Flash)\r\n", (unsigned long)value);
                } else {
                    snprintf(response, sizeof(response), "PWM duty set to %lu (Flash save failed)\r\n", (unsigned long)value);
                }
                cli_send_string(response);
            }
            else {
                cli_send_string("Error: Invalid value. Range: 0-500\r\n");
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

        case CLI_CMD_GET_START_SIGNAL:
        {
            GPIO_PinState pin_state = HAL_GPIO_ReadPin(MOTOR_PM_ENABLE_GPIO_Port, MOTOR_PM_ENABLE_Pin);
            snprintf(response, sizeof(response), "Current MOTOR_PM_ENABLE pin level: %d (%s)\r\n", 
                     (int)pin_state, (pin_state == GPIO_PIN_SET) ? "HIGH" : "LOW");
            cli_send_string(response);
        }
            break;

        case CLI_CMD_SET_MAX_SPEED:
            if (param == NULL || strlen(param) == 0) {
                cli_send_string("Error: Missing value. Usage: set max_speed <rpm>\r\n");
                break;
            }
            value = atoi(param);
            if (value > 0 && value <= 10000) {
                if (pusher_motor_set_max_speed((uint32_t)value) == 0) {
                    snprintf(response, sizeof(response), "Max speed set to %lu RPM (saved to Flash)\r\n", (unsigned long)value);
                } else {
                    snprintf(response, sizeof(response), "Max speed set to %lu RPM (Flash save failed)\r\n", (unsigned long)value);
                }
                cli_send_string(response);
            }
            else {
                cli_send_string("Error: Invalid value. Range: 1-10000 RPM\r\n");
            }
            break;

        case CLI_CMD_SET_SPEED:
            if (param == NULL || strlen(param) == 0) {
                // 计算当前最高转速下的最大速度
                float circumference = 3.14159f * 6.0f;
                uint32_t max_speed = (uint32_t)((float)pusher_motor_get_max_speed() * circumference);
                snprintf(response, sizeof(response), "Error: Missing value. Usage: set speed <cm/min> (Range: 0-%lu cm/min)\r\n", (unsigned long)max_speed);
                cli_send_string(response);
                break;
            }
            value = atoi(param);
            if (value >= 0) {
                if (pusher_motor_set_speed((uint32_t)value) == 0) {
                    uint32_t duty = pusher_motor_calculate_duty_from_speed((uint32_t)value);
                    // 计算当前最高转速下的最大速度
                    float circumference = 3.14159f * 6.0f;
                    uint32_t max_speed = (uint32_t)((float)pusher_motor_get_max_speed() * circumference);
                    snprintf(response, sizeof(response), "Speed set to %lu cm/min (PWM duty: %lu) (Range: 0-%lu cm/min) (saved to Flash)\r\n",
                            (unsigned long)value, (unsigned long)duty, (unsigned long)max_speed);
                } else {
                    snprintf(response, sizeof(response), "Speed set to %lu cm/min (Flash save failed)\r\n", (unsigned long)value);
                }
                cli_send_string(response);
            }
            else {
                // 计算当前最高转速下的最大速度
                float circumference = 3.14159f * 6.0f;
                uint32_t max_speed = (uint32_t)((float)pusher_motor_get_max_speed() * circumference);
                snprintf(response, sizeof(response), "Error: Invalid value. Must be non-negative (Range: 0-%lu cm/min)\r\n", (unsigned long)max_speed);
                cli_send_string(response);
            }
            break;

        case CLI_CMD_SET_WAIT_TIME:
            if (param == NULL || strlen(param) == 0) {
                cli_send_string("Error: Missing value. Usage: set wait_time <ms>\r\n");
                break;
            }
            value = atoi(param);
            if (value >= 0 && value <= 10000) {
                if (pusher_motor_set_wait_time((uint32_t)value) == 0) {
                    snprintf(response, sizeof(response), "Wait time set to %lu ms (saved to Flash)\r\n", (unsigned long)value);
                } else {
                    snprintf(response, sizeof(response), "Wait time set to %lu ms (Flash save failed)\r\n", (unsigned long)value);
                }
                cli_send_string(response);
            }
            else {
                cli_send_string("Error: Invalid value. Range: 0-10000 ms\r\n");
            }
            break;

        case CLI_CMD_SET_MOTOR_MP_A_DIR:
            if (param == NULL || strlen(param) == 0) {
                cli_send_string("Error: Missing value. Usage: set motor_mp_a_dir <0|1>\r\n");
                break;
            }
            value = atoi(param);
            if (value == 0 || value == 1) {
                if (pusher_motor_set_motor_mp_a_dir((uint32_t)value) == 0) {
                    snprintf(response, sizeof(response), "Motor MP A direction set to %lu (saved to Flash)\r\n", (unsigned long)value);
                } else {
                    snprintf(response, sizeof(response), "Motor MP A direction set to %lu (Flash save failed)\r\n", (unsigned long)value);
                }
                cli_send_string(response);
            }
            else {
                cli_send_string("Error: Invalid value. Must be 0 or 1\r\n");
            }
            break;

        case CLI_CMD_SET_MOTOR_MP_B_DIR:
            if (param == NULL || strlen(param) == 0) {
                cli_send_string("Error: Missing value. Usage: set motor_mp_b_dir <0|1>\r\n");
                break;
            }
            value = atoi(param);
            if (value == 0 || value == 1) {
                if (pusher_motor_set_motor_mp_b_dir((uint32_t)value) == 0) {
                    snprintf(response, sizeof(response), "Motor MP B direction set to %lu (saved to Flash)\r\n", (unsigned long)value);
                } else {
                    snprintf(response, sizeof(response), "Motor MP B direction set to %lu (Flash save failed)\r\n", (unsigned long)value);
                }
                cli_send_string(response);
            }
            else {
                cli_send_string("Error: Invalid value. Must be 0 or 1\r\n");
            }
            break;

        case CLI_CMD_SET_ACCELERATION:
            if (param == NULL || strlen(param) == 0) {
                cli_send_string("Error: Missing value. Usage: set acceleration <0-50>\r\n");
                break;
            }
            value = atoi(param);
            if (value >= 0 && value <= 50) {
                if (pusher_motor_set_acceleration((uint8_t)value) == 0) {
                    snprintf(response, sizeof(response), "Acceleration set to %lu (0=off)\r\n", (unsigned long)value);
                } else {
                    snprintf(response, sizeof(response), "Acceleration set to %lu failed\r\n", (unsigned long)value);
                }
                cli_send_string(response);
            }
            else {
                cli_send_string("Error: Invalid value. Range: 0-50\r\n");
            }
            break;

        case CLI_CMD_GET_ACCELERATION:
            snprintf(response, sizeof(response), "Current acceleration: %u (0=off, higher=faster ramp)\r\n", (unsigned int)pusher_motor_get_acceleration());
            cli_send_string(response);
            break;

        case CLI_CMD_HELP:
            cli_send_string("\r\nAvailable commands:\r\n");
            cli_send_string("  start                    - Start pusher motor\r\n");
            cli_send_string("  set direction_time <ms>  - Set direction time (1-60000 ms)\r\n");
            cli_send_string("  set pwm_duty <value>     - Set PWM duty cycle (0-500)\r\n");
            cli_send_string("  set wait_time <ms>       - Set wait time (0-10000 ms)\r\n");
            cli_send_string("  set max_speed <rpm>      - Set max speed (1-10000 RPM)\r\n");
            cli_send_string("  set speed <cm/min>       - Set speed (0+ cm/min)\r\n");
            cli_send_string("  set motor_mp_a_dir <0|1> - Set motor MP A direction\r\n");
            cli_send_string("  set motor_mp_b_dir <0|1> - Set motor MP B direction\r\n");
            cli_send_string("  set new_pwm_duty <value> - Directly set PWM duty cycle (0-500, not saved)\r\n");
            cli_send_string("  set acceleration <0-50>  - Set acceleration step (0=off)\r\n");
            cli_send_string("  get direction_time       - Get current direction time\r\n");
            cli_send_string("  get pwm_duty             - Get current PWM duty cycle\r\n");
            cli_send_string("  get wait_time            - Get current wait time\r\n");
            cli_send_string("  get max_speed            - Get current max speed\r\n");
            cli_send_string("  get speed                - Get current speed\r\n");
            cli_send_string("  get motor_mp_a_dir       - Get current motor MP A direction\r\n");
            cli_send_string("  get motor_mp_b_dir       - Get current motor MP B direction\r\n");
            cli_send_string("  get start_signal         - Get MOTOR_PM_ENABLE pin level (PB6)\r\n");
            cli_send_string("  get acceleration         - Get current acceleration step\r\n");
            cli_send_string("  help / ?                 - Show this help\r\n");
            cli_send_string("\r\n");
            break;

        case CLI_CMD_UNKNOWN:
        default:
            cli_send_string("Unknown command. Type 'help' for available commands.\r\n");
            break;
    }
}

/**
 * @brief 检查串口是否有数据可读（非阻塞）
 * @return 1: 有数据可读, 0: 无数据可读
 */
static uint8_t cli_uart_readable(void)
{
    if (cli_config.huart == NULL) return 0;
    return (__HAL_UART_GET_FLAG(cli_config.huart, UART_FLAG_RXNE) != RESET) ? 1 : 0;
}

/**
 * @brief 读取串口数据（非阻塞）
 * @return 读取到的数据
 */
static uint8_t cli_uart_read(void)
{
    if (cli_config.huart == NULL) return 0;
    return (uint8_t)(cli_config.huart->Instance->DR & 0xFF);
}

/**
 * @brief 从串口接收数据到环形缓冲区
 * @note 尽可能快地读取所有可用数据
 */
static void cli_receive_data(void)
{
    while (cli_uart_readable()) {
        uint8_t data = cli_uart_read();
        rx_buf_write(data);
    }
}

/**
 * @brief 回显字符（放入发送缓冲区，非阻塞）
 */
static void cli_echo(uint8_t data)
{
    // 处理特殊字符的回显
    if (data == '\r' || data == '\n') {
        cli_puts("\r\n");
    }
    else if (data == '\b' || data == 0x7F) {  // Backspace or DEL
        cli_puts("\b \b");
    }
    else if (data >= 0x20 && data < 0x7F) {  // 可打印字符
        cli_putchar(data);
    }
}

/**
 * @brief 处理退格键
 */
static void handle_backspace(void)
{
    if (cmd_len > 0) {
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
    // 处理行结束符
    if (data == '\r' || data == '\n') {
        if (cmd_len > 0) {
            cmd_buf[cmd_len] = '\0';
            return 1;  // 命令完成
        }
        return 0;  // 空行，忽略
    }
    
    // 处理退格键
    if (data == '\b' || data == 0x7F) {
        handle_backspace();
        return 0;
    }
    
    // 忽略控制字符（除了上面处理的）
    if (data < 0x20 || data >= 0x7F) {
        return 0;
    }
    
    // 存储可打印字符
    if (cmd_len < RX_BUF_SIZE - 1) {
        cmd_buf[cmd_len++] = data;
        cmd_buf[cmd_len] = '\0';
        cli_echo(data);
    }
    
    return 0;
}

/**
 * @brief CLI 处理函数
 * @note 应在主循环中周期性调用，使用非阻塞轮询方式
 */
void cli_process(void)
{
    // 第一步：尽可能快地读取所有串口数据到环形缓冲区
    cli_receive_data();
    
    // 第二步：处理发送缓冲区（非阻塞回显）
    cli_process_tx();
    
    // 第三步：从接收缓冲区处理数据
    uint8_t data;
    while (rx_buf_read(&data)) {
        // 处理输入字符
        if (process_input_line(data)) {
            // 命令接收完成，解析并执行
            CliCommand_t cmd;
            char *param = NULL;
            
            // 复制命令字符串以便修改
            char cmd_str[RX_BUF_SIZE];
            strncpy(cmd_str, (char *)cmd_buf, sizeof(cmd_str) - 1);
            cmd_str[sizeof(cmd_str) - 1] = '\0';
            
            // 解析并执行命令
            int parse_result = parse_command(cmd_str, &cmd, &param);
            if (parse_result == 0) {
                cli_execute_command(cmd, param);
            }
            else if (parse_result == -2) {
                // 命令已在解析时直接处理（如set new_pwm_duty），只需清理缓冲区
            }
            else {
                cli_execute_command(CLI_CMD_UNKNOWN, NULL);
            }

            // 重置命令缓冲区
            cmd_len = 0;
            memset(cmd_buf, 0, sizeof(cmd_buf));

            // 发送提示符（放入发送缓冲区）- 如果命令未直接发送提示符
            if (parse_result != -2) {
                cli_puts("> ");
            }

            // 命令处理完成，退出本次处理，让发送有机会执行
            break;
        }
    }
    
    // 第四步：再次处理发送缓冲区，确保回显及时输出
    cli_process_tx();
}
