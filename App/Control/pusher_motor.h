#ifndef __PUSHER_MOTOR_H__
#define __PUSHER_MOTOR_H__

#include "main.h"

// 定义推料电机的定时器和通道
#define PUSHER_MOTOR_PWM_TIMER htim4
#define PUSHER_MOTOR_PWM_CHANNEL TIM_CHANNEL_4
#define PUSHER_MOTOR_MAX_DUTY 500

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

/**
 * @brief 根据速度（cm/分钟）计算PWM占空比
 * @param speed_cm_min 速度（cm/分钟）
 * @return PWM占空比（0-500）
 */
uint32_t pusher_motor_calculate_duty_from_speed(uint32_t speed_cm_min);

/**
 * @brief 设置最高转速
 * @param max_speed_rpm 最高转速（RPM）
 * @return 0: 成功, 其他: 失败
 */
uint32_t pusher_motor_set_max_speed(uint32_t max_speed_rpm);

/**
 * @brief 根据速度（cm/分钟）设置PWM占空比
 * @param speed_cm_min 速度（cm/分钟）
 * @return 0: 成功, 其他: 失败
 */
uint32_t pusher_motor_set_speed(uint32_t speed_cm_min);

/**
 * @brief 根据当前占空比和最高转速计算当前速度（cm/分钟）
 * @return 当前速度（cm/分钟）
 */
uint32_t pusher_motor_calculate_speed_from_duty(void);

/**
 * @brief 设置运行时间
 * @param time_ms 运行时间（毫秒）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t pusher_motor_set_direction_time(uint32_t time_ms);

/**
 * @brief 设置等待时间
 * @param time_ms 等待时间（毫秒）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t pusher_motor_set_wait_time(uint32_t time_ms);

/**
 * @brief 设置PWM占空比
 * @param duty PWM占空比（0-500）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t pusher_motor_set_pwm_duty(uint32_t duty);

/**
 * @brief 直接设置PWM占空比（不保存到Flash）
 * @param duty PWM占空比（0-500）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t pusher_motor_set_pwm_duty_direct(uint32_t duty);

/**
 * @brief 设置电机A方向
 * @param dir 方向（0或1）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t pusher_motor_set_motor_mp_a_dir(uint32_t dir);

/**
 * @brief 设置电机B方向
 * @param dir 方向（0或1）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t pusher_motor_set_motor_mp_b_dir(uint32_t dir);

/**
 * @brief 获取运行时间
 * @return 运行时间（毫秒）
 */
uint32_t pusher_motor_get_direction_time(void);

/**
 * @brief 获取等待时间
 * @return 等待时间（毫秒）
 */
uint32_t pusher_motor_get_wait_time(void);

/**
 * @brief 获取PWM占空比
 * @return PWM占空比（0-500）
 */
uint32_t pusher_motor_get_pwm_duty(void);

/**
 * @brief 获取最高转速
 * @return 最高转速（RPM）
 */
uint32_t pusher_motor_get_max_speed(void);

/**
 * @brief 获取电机A方向
 * @return 方向（0或1）
 */
uint32_t pusher_motor_get_motor_mp_a_dir(void);

/**
 * @brief 获取电机B方向
 * @return 方向（0或1）
 */
uint32_t pusher_motor_get_motor_mp_b_dir(void);

/**
 * @brief 获取启动信号电平
 * @return 电平值（0或1）
 */
uint32_t pusher_motor_get_start_signal(void);

/**
 * @brief 设置加速度值
 * @param accel 加速度值（0-50），0=无加速
 * @return 0: 成功, 1: 参数无效
 */
uint32_t pusher_motor_set_acceleration(uint8_t accel);

/**
 * @brief 获取当前加速度值
 * @return 加速度值（0-50）
 */
uint8_t pusher_motor_get_acceleration(void);

#endif
