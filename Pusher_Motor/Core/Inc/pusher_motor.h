#ifndef __PUSHER_MOTOR_H__
#define __PUSHER_MOTOR_H__

#include "main.h"

// 外部变量声明
extern uint32_t MOTOR_DIRECTION_TIME_MS;
extern uint32_t MOTOR_PWM_DUTY;

/**
 * @brief 推料电机初始化函数
 * @details 从Flash读取参数，开启PWM输出并设置占空比为0%
 */
void pusher_motor_init(void);

/**
 * @brief 推料电机启动函数
 * @details 开始推料操作
 */
void pusher_motor_start(void);

/**
 * @brief 推料电机循环函数
 * @details 处理推料电机的循环逻辑
 */
void pusher_motor_loop(void);

/**
 * @brief 保存参数到Flash
 * @return HAL_StatusTypeDef
 */
uint32_t pusher_motor_save_params(void);

#endif
