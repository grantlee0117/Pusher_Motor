/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : params_manager.c
 * @brief          : 参数管理模块实现
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

#include "params_manager.h"
#include "flash_storage.h"

// 默认参数值
#define DEFAULT_DIRECTION_TIME_MS 250 // 默认运行时间，单位：毫秒
#define DEFAULT_PWM_DUTY 150          // 默认PWM占空比，500最大
#define DEFAULT_PWM_DUTY_MAX 500      // PWM最大占空比
#define DEFAULT_WAIT_TIME_MS 250      // 默认等待时间，单位：毫秒
#define DEFAULT_ACCEL_TIME_MS 0       // 默认加速时间，0表示保持原有瞬时启动
#define DEFAULT_MAX_SPEED_RPM 3655    // 默认最高转速，单位：RPM
#define DEFAULT_MOTOR_MP_A_DIR 1      // 默认电机A方向（低电平）
#define DEFAULT_MOTOR_MP_B_DIR 0      // 默认电机B方向（高电平）

// 静态参数结构体
static MotorParams_t motor_params = {
    .direction_time_ms = DEFAULT_DIRECTION_TIME_MS,
    .pwm_duty = DEFAULT_PWM_DUTY,
    .pwm_duty_max = DEFAULT_PWM_DUTY_MAX,
    .wait_time_ms = DEFAULT_WAIT_TIME_MS,
    .accel_time_ms = DEFAULT_ACCEL_TIME_MS,
    .max_speed_rpm = DEFAULT_MAX_SPEED_RPM,
    .motor_mp_a_dir = DEFAULT_MOTOR_MP_A_DIR,
    .motor_mp_b_dir = DEFAULT_MOTOR_MP_B_DIR};

/**
 * @brief 初始化参数管理器
 * @return 0: 成功, 1: 使用默认值
 */
uint32_t params_manager_init(void)
{
    return params_manager_load();
}

/**
 * @brief 获取所有参数
 * @param params 参数结构体指针
 */
void params_manager_get_all(MotorParams_t *params)
{
    if (params)
    {
        *params = motor_params;
    }
}

/**
 * @brief 设置运行时间
 * @param time_ms 运行时间（毫秒）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t params_manager_set_direction_time(uint32_t time_ms)
{
    if (time_ms == 0 || time_ms > 60000)
    {
        return 1; // 参数无效
    }

    motor_params.direction_time_ms = time_ms;
    return 0;
}

/**
 * @brief 获取运行时间
 * @return 运行时间（毫秒）
 */
uint32_t params_manager_get_direction_time(void)
{
    return motor_params.direction_time_ms;
}

/**
 * @brief 设置PWM占空比
 * @param duty PWM占空比（0-500）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t params_manager_set_pwm_duty(uint32_t duty)
{
    if (duty > motor_params.pwm_duty_max)
    {
        return 1; // 参数无效
    }

    motor_params.pwm_duty = duty;
    return 0;
}

/**
 * @brief 获取PWM占空比
 * @return PWM占空比（0-500）
 */
uint32_t params_manager_get_pwm_duty(void)
{
    return motor_params.pwm_duty;
}

/**
 * @brief 设置等待时间
 * @param time_ms 等待时间（毫秒）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t params_manager_set_wait_time(uint32_t time_ms)
{
    if (time_ms > 10000)
    {
        return 1; // 参数无效
    }

    motor_params.wait_time_ms = time_ms;
    return 0;
}

/**
 * @brief 获取等待时间
 * @return 等待时间（毫秒）
 */
uint32_t params_manager_get_wait_time(void)
{
    return motor_params.wait_time_ms;
}

/**
 * @brief 设置加速时间
 * @param time_ms 加速时间（毫秒）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t params_manager_set_accel_time(uint32_t time_ms)
{
    if (time_ms > 10000)
    {
        return 1; // 参数无效
    }

    motor_params.accel_time_ms = time_ms;
    return 0;
}

/**
 * @brief 获取加速时间
 * @return 加速时间（毫秒）
 */
uint32_t params_manager_get_accel_time(void)
{
    return motor_params.accel_time_ms;
}

/**
 * @brief 设置最高转速
 * @param max_speed_rpm 最高转速（RPM）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t params_manager_set_max_speed(uint32_t max_speed_rpm)
{
    if (max_speed_rpm == 0 || max_speed_rpm > 10000)
    {
        return 1; // 参数无效
    }

    motor_params.max_speed_rpm = max_speed_rpm;
    return 0;
}

/**
 * @brief 获取最高转速
 * @return 最高转速（RPM）
 */
uint32_t params_manager_get_max_speed(void)
{
    return motor_params.max_speed_rpm;
}

/**
 * @brief 设置电机A方向
 * @param dir 方向（0或1）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t params_manager_set_motor_mp_a_dir(uint32_t dir)
{
    if (dir != 0 && dir != 1)
    {
        return 1; // 参数无效
    }

    motor_params.motor_mp_a_dir = dir;
    return 0;
}

/**
 * @brief 获取电机A方向
 * @return 方向（0或1）
 */
uint32_t params_manager_get_motor_mp_a_dir(void)
{
    return motor_params.motor_mp_a_dir;
}

/**
 * @brief 设置电机B方向
 * @param dir 方向（0或1）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t params_manager_set_motor_mp_b_dir(uint32_t dir)
{
    if (dir != 0 && dir != 1)
    {
        return 1; // 参数无效
    }

    motor_params.motor_mp_b_dir = dir;
    return 0;
}

/**
 * @brief 获取电机B方向
 * @return 方向（0或1）
 */
uint32_t params_manager_get_motor_mp_b_dir(void)
{
    return motor_params.motor_mp_b_dir;
}

/**
 * @brief 保存参数到Flash
 * @return 0: 成功, 其他: 失败
 */
uint32_t params_manager_save(void)
{
    FlashStorage_t storage;

    storage.direction_time_ms = motor_params.direction_time_ms;
    storage.pwm_duty = motor_params.pwm_duty;
    storage.wait_time_ms = motor_params.wait_time_ms;
    storage.accel_time_ms = motor_params.accel_time_ms;
    storage.max_speed_rpm = motor_params.max_speed_rpm;
    storage.motor_mp_a_dir = motor_params.motor_mp_a_dir;
    storage.motor_mp_b_dir = motor_params.motor_mp_b_dir;

    return FlashStorage_Write(&storage);
}

/**
 * @brief 从Flash加载参数
 * @return 0: 成功, 1: 使用默认值
 */
uint32_t params_manager_load(void)
{
    FlashStorage_t storage;

    // 尝试从Flash读取参数
    if (FlashStorage_Read(&storage) == HAL_OK && FlashStorage_IsValid(&storage))
    {
        // 读取成功，更新参数
        motor_params.direction_time_ms = storage.direction_time_ms;
        motor_params.pwm_duty = storage.pwm_duty;
        motor_params.wait_time_ms = storage.wait_time_ms;
        motor_params.accel_time_ms = storage.accel_time_ms;
        motor_params.max_speed_rpm = storage.max_speed_rpm;
        motor_params.motor_mp_a_dir = storage.motor_mp_a_dir;
        motor_params.motor_mp_b_dir = storage.motor_mp_b_dir;

        return 0; // 成功
    }
    else
    {
        // 读取失败时只使用RAM中的工程默认值，不在启动阶段擦写Flash。
        // 参数会在用户执行保存命令时再写入Flash。
        return 1; // 使用默认值
    }
}
