/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : params_manager.h
  * @brief          : 参数管理模块头文件
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef PARAMS_MANAGER_H
#define PARAMS_MANAGER_H

#include <stdint.h>

/**
 * @brief 推料电机参数结构体
 */
typedef struct {
    uint32_t direction_time_ms;  // 运行时间（毫秒）
    uint32_t pwm_duty;            // PWM占空比（0-500）
    uint32_t pwm_duty_max;        // PWM最大占空比
    uint32_t wait_time_ms;         // 等待时间（毫秒）
    uint32_t max_speed_rpm;       // 最高转速（RPM）
    uint32_t motor_mp_a_dir;      // 电机A方向
    uint32_t motor_mp_b_dir;      // 电机B方向
} MotorParams_t;

/**
 * @brief 初始化参数管理器
 * @return 0: 成功, 1: 使用默认值
 */
uint32_t params_manager_init(void);

/**
 * @brief 获取所有参数
 * @param params 参数结构体指针
 */
void params_manager_get_all(MotorParams_t *params);

/**
 * @brief 设置运行时间
 * @param time_ms 运行时间（毫秒）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t params_manager_set_direction_time(uint32_t time_ms);

/**
 * @brief 获取运行时间
 * @return 运行时间（毫秒）
 */
uint32_t params_manager_get_direction_time(void);

/**
 * @brief 设置PWM占空比
 * @param duty PWM占空比（0-500）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t params_manager_set_pwm_duty(uint32_t duty);

/**
 * @brief 获取PWM占空比
 * @return PWM占空比（0-500）
 */
uint32_t params_manager_get_pwm_duty(void);

/**
 * @brief 设置等待时间
 * @param time_ms 等待时间（毫秒）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t params_manager_set_wait_time(uint32_t time_ms);

/**
 * @brief 获取等待时间
 * @return 等待时间（毫秒）
 */
uint32_t params_manager_get_wait_time(void);

/**
 * @brief 设置最高转速
 * @param max_speed_rpm 最高转速（RPM）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t params_manager_set_max_speed(uint32_t max_speed_rpm);

/**
 * @brief 获取最高转速
 * @return 最高转速（RPM）
 */
uint32_t params_manager_get_max_speed(void);

/**
 * @brief 设置电机A方向
 * @param dir 方向（0或1）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t params_manager_set_motor_mp_a_dir(uint32_t dir);

/**
 * @brief 获取电机A方向
 * @return 方向（0或1）
 */
uint32_t params_manager_get_motor_mp_a_dir(void);

/**
 * @brief 设置电机B方向
 * @param dir 方向（0或1）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t params_manager_set_motor_mp_b_dir(uint32_t dir);

/**
 * @brief 获取电机B方向
 * @return 方向（0或1）
 */
uint32_t params_manager_get_motor_mp_b_dir(void);

/**
 * @brief 保存参数到Flash
 * @return 0: 成功, 其他: 失败
 */
uint32_t params_manager_save(void);

/**
 * @brief 从Flash加载参数
 * @return 0: 成功, 1: 使用默认值
 */
uint32_t params_manager_load(void);

#endif /* PARAMS_MANAGER_H */
