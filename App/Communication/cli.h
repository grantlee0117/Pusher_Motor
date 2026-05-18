#ifndef __CLI_H__
#define __CLI_H__

#include "main.h"

// CLI 命令定义
typedef enum {
    CLI_CMD_START,              // 启动推料电机
    CLI_CMD_SET_DIRECTION_TIME, // 设置方向运行时间
    CLI_CMD_SET_PWM_DUTY,       // 设置PWM占空比
    CLI_CMD_SET_WAIT_TIME,      // 设置等待时间
    CLI_CMD_SET_MAX_SPEED,      // 设置最高转速
    CLI_CMD_SET_SPEED,          // 设置速度（cm/分钟）
    CLI_CMD_SET_MOTOR_MP_A_DIR, // 设置电机A方向引脚状态
    CLI_CMD_SET_MOTOR_MP_B_DIR, // 设置电机B方向引脚状态
    CLI_CMD_GET_DIRECTION_TIME, // 获取方向运行时间
    CLI_CMD_GET_PWM_DUTY,       // 获取PWM占空比
    CLI_CMD_GET_WAIT_TIME,      // 获取等待时间
    CLI_CMD_GET_MAX_SPEED,      // 获取最高转速
    CLI_CMD_GET_SPEED,          // 获取当前速度（cm/分钟）
    CLI_CMD_GET_MOTOR_MP_A_DIR, // 获取电机A方向引脚状态
    CLI_CMD_GET_MOTOR_MP_B_DIR, // 获取电机B方向引脚状态
    CLI_CMD_GET_START_SIGNAL,   // 获取启动信号电平
    CLI_CMD_SET_NEW_PWM_DUTY,   // 直接更改当前占空比（不保存Flash）
    CLI_CMD_SET_ACCELERATION,   // 设置加速度值
    CLI_CMD_GET_ACCELERATION,   // 获取加速度值
    CLI_CMD_HELP,               // 帮助命令
    CLI_CMD_UNKNOWN             // 未知命令
} CliCommand_t;

// CLI 状态定义
typedef enum {
    CLI_STATE_IDLE,             // 空闲状态
    CLI_STATE_PARSING           // 解析命令状态
} CliState_t;

// CLI 配置结构体
typedef struct {
    UART_HandleTypeDef *huart;  // 串口句柄
    uint8_t rx_buffer[256];     // 接收缓冲区（增加大小）
    uint16_t rx_index;          // 接收缓冲区索引
    CliState_t state;           // CLI状态
} CliConfig_t;

// 外部变量声明
extern CliConfig_t cli_config;

// 函数声明
void cli_init(UART_HandleTypeDef *huart);
void cli_process(void);

void cli_send_string(const char *str);

#endif /* __CLI_H__ */