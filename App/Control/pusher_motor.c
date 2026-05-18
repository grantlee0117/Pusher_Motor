#include "pusher_motor.h"
#include "params_manager.h"

extern TIM_HandleTypeDef htim4;

/* 加速相关变量 */
static uint16_t accel_current_duty = 500; // 当前实际输出的占空比
static uint32_t last_accel_tick = 0;      // 上次调整占空比的时间戳
static uint8_t acc_speed_value = 0;       // 加速度值（0-50），0=无加速
#define ACCEL_INTERVAL_MS 10              // 每 10ms 调整一次占空比

// 推料电机状态定义
typedef enum
{
    MOTOR_STATE_IDLE = 0, // 空闲状态
    MOTOR_STATE_WAIT,     // 等待状态
    MOTOR_STATE_RUNNING,  // 运行状态
    MOTOR_STATE_STOP      // 停止状态
} MotorState_t;

// 推料电机控制变量
MotorState_t motor_state = MOTOR_STATE_IDLE;
uint32_t motor_start_time = 0;

// 开始工作标志位
volatile uint8_t pusher_motor_start_flag = 0;
// 工作标志位
uint8_t pusher_motor_work_flag = 0;

void pusher_motor_init(void)
{
    // 初始化参数管理器
    params_manager_init();

    // 初始化PWM输出
    HAL_TIM_PWM_Start(&PUSHER_MOTOR_PWM_TIMER, PUSHER_MOTOR_PWM_CHANNEL);
    __HAL_TIM_SetCompare(&PUSHER_MOTOR_PWM_TIMER, PUSHER_MOTOR_PWM_CHANNEL, PUSHER_MOTOR_MAX_DUTY);

    // 根据参数设置方向引脚电平
    HAL_GPIO_WritePin(MOTOR_PM_A_DIRECTION_GPIO_Port, MOTOR_PM_A_DIRECTION_Pin,
                      (GPIO_PinState)params_manager_get_motor_mp_a_dir());
    HAL_GPIO_WritePin(MOTOR_PM_B_DIRECTION_GPIO_Port, MOTOR_PM_B_DIRECTION_Pin,
                      (GPIO_PinState)params_manager_get_motor_mp_b_dir());

    // 初始化状态
    motor_state = MOTOR_STATE_IDLE;
}

void pusher_motor_start(void)
{
    pusher_motor_start_flag = 1;
}

void pusher_motor_loop(void)
{
    uint32_t current_time = HAL_GetTick();

    switch (motor_state)
    {
    case MOTOR_STATE_IDLE:
        // 检查是否有启动信号
        if (pusher_motor_start_flag)
        {
            // 清除启动标志
            pusher_motor_start_flag = 0;
            // 记录等待开始时间
            motor_start_time = current_time;
            // 进入等待状态
            motor_state = MOTOR_STATE_WAIT;
        }
        break;

    case MOTOR_STATE_WAIT:
        // 检查等待时间
        if ((current_time - motor_start_time) >= params_manager_get_wait_time())
        {
            // 设置工作标志
            pusher_motor_work_flag = 1;
            // 记录运行开始时间
            motor_start_time = current_time;
            last_accel_tick = current_time;

            if (acc_speed_value == 0)
            {
                // 无加速：直接输出目标占空比
                __HAL_TIM_SetCompare(&PUSHER_MOTOR_PWM_TIMER, PUSHER_MOTOR_PWM_CHANNEL,
                                     params_manager_get_pwm_duty());
            }
            else
            {
                // 加速模式：从停止状态（500）开始
                accel_current_duty = 500;
                __HAL_TIM_SetCompare(&PUSHER_MOTOR_PWM_TIMER, PUSHER_MOTOR_PWM_CHANNEL, 500);
            }

            // 进入运行状态
            motor_state = MOTOR_STATE_RUNNING;
        }
        break;

    case MOTOR_STATE_RUNNING:
        // 检查运行时间
        if ((current_time - motor_start_time) >= params_manager_get_direction_time())
        {
            // 运行结束，进入停止状态
            motor_state = MOTOR_STATE_STOP;
        }
        else if (acc_speed_value != 0)
        {
            uint32_t target_duty = params_manager_get_pwm_duty();

            if (accel_current_duty > target_duty)
            {
                // 每隔 ACCEL_INTERVAL_MS 调整一次，避免变化过快
                if ((current_time - last_accel_tick) >= ACCEL_INTERVAL_MS)
                {
                    last_accel_tick = current_time;

                    if (accel_current_duty - target_duty > acc_speed_value)
                    {
                        accel_current_duty -= acc_speed_value;
                    }
                    else
                    {
                        accel_current_duty = (uint16_t)target_duty; // 到达目标
                    }

                    __HAL_TIM_SetCompare(&PUSHER_MOTOR_PWM_TIMER, PUSHER_MOTOR_PWM_CHANNEL,
                                         accel_current_duty);
                }
            }
        }
        break;

    case MOTOR_STATE_STOP:
        // 电机停止
        __HAL_TIM_SetCompare(&PUSHER_MOTOR_PWM_TIMER, PUSHER_MOTOR_PWM_CHANNEL, 500);
        // 重置加速占空比
        accel_current_duty = 500;
        // 清除标志
        pusher_motor_work_flag = 0;
        pusher_motor_start_flag = 0;
        // 回到空闲状态
        motor_state = MOTOR_STATE_IDLE;
        break;
    }
}

/**
 * @brief 保存参数到Flash
 * @return HAL_StatusTypeDef
 */
uint32_t pusher_motor_save_params(void)
{
    return params_manager_save();
}

/**
 * @brief 根据速度（cm/分钟）计算PWM占空比
 * @param speed_cm_min 速度（cm/分钟）
 * @return PWM占空比（0-500）
 */
uint32_t pusher_motor_calculate_duty_from_speed(uint32_t speed_cm_min)
{
    // 电机直径：6cm，周长 = π * 直径 = 3.14159 * 6 ≈ 18.8496 cm
    float circumference = 3.14159f * 6.0f;

    // 计算对应的转速（RPM）
    // 转速 = 速度 / 周长
    float speed_rpm = (float)speed_cm_min / circumference;

    // 确保转速在有效范围内
    if (speed_rpm <= 0)
    {
        return PUSHER_MOTOR_MAX_DUTY; // 0 RPM，最大占空比
    }
    if (speed_rpm >= params_manager_get_max_speed())
    {
        return 0; // 最高转速，最小占空比
    }

    // 线性计算PWM占空比
    // 占空比 = (最高转速 - 当前转速) / 最高转速 * 最大占空比
    uint32_t duty = (uint32_t)(((float)params_manager_get_max_speed() - speed_rpm) /
                               (float)params_manager_get_max_speed() * 500.0f);

    // 确保占空比在有效范围内
    if (duty > PUSHER_MOTOR_MAX_DUTY)
    {
        duty = PUSHER_MOTOR_MAX_DUTY;
    }

    return duty;
}

/**
 * @brief 设置最高转速
 * @param max_speed_rpm 最高转速（RPM）
 * @return 0: 成功, 其他: 失败
 */
uint32_t pusher_motor_set_max_speed(uint32_t max_speed_rpm)
{
    // 设置最高转速
    if (params_manager_set_max_speed(max_speed_rpm) != 0)
    {
        return 1; // 参数无效
    }

    // 保存到Flash
    return params_manager_save();
}

/**
 * @brief 根据速度（cm/分钟）设置PWM占空比
 * @param speed_cm_min 速度（cm/分钟）
 * @return 0: 成功, 其他: 失败
 */
uint32_t pusher_motor_set_speed(uint32_t speed_cm_min)
{
    // 计算PWM占空比
    uint32_t duty = pusher_motor_calculate_duty_from_speed(speed_cm_min);

    // 更新PWM占空比
    if (params_manager_set_pwm_duty(duty) != 0)
    {
        return 1; // 参数无效
    }

    // 保存到Flash
    return params_manager_save();
}

/**
 * @brief 根据当前占空比和最高转速计算当前速度（cm/分钟）
 * @return 当前速度（cm/分钟）
 */
uint32_t pusher_motor_calculate_speed_from_duty(void)
{
    // 电机直径：6cm，周长 = π * 直径 = 3.14159 * 6 ≈ 18.8496 cm
    float circumference = 3.14159f * 6.0f;

    // 计算当前转速（RPM）
    // 转速 = (1 - 占空比 / 最大占空比) * 最高转速
    float speed_rpm = (1.0f - (float)params_manager_get_pwm_duty() / 500.0f) *
                      (float)params_manager_get_max_speed();

    // 计算速度（cm/分钟）
    // 速度 = 转速 * 周长
    float speed_cm_min = speed_rpm * circumference;

    // 确保速度非负
    if (speed_cm_min < 0)
    {
        speed_cm_min = 0;
    }

    return (uint32_t)speed_cm_min;
}

/**
 * @brief 设置运行时间
 * @param time_ms 运行时间（毫秒）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t pusher_motor_set_direction_time(uint32_t time_ms)
{
    if (params_manager_set_direction_time(time_ms) != 0)
    {
        return 1; // 参数无效
    }

    return params_manager_save();
}

/**
 * @brief 设置等待时间
 * @param time_ms 等待时间（毫秒）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t pusher_motor_set_wait_time(uint32_t time_ms)
{
    if (params_manager_set_wait_time(time_ms) != 0)
    {
        return 1; // 参数无效
    }

    return params_manager_save();
}

/**
 * @brief 设置PWM占空比
 * @param duty PWM占空比（0-500）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t pusher_motor_set_pwm_duty(uint32_t duty)
{
    if (params_manager_set_pwm_duty(duty) != 0)
    {
        return 1; // 参数无效
    }

    return params_manager_save();
}

/**
 * @brief 直接设置PWM占空比（不保存到Flash）
 * @param duty PWM占空比（0-500）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t pusher_motor_set_pwm_duty_direct(uint32_t duty)
{
    if (duty > PUSHER_MOTOR_MAX_DUTY)
    {
        return 1; // 参数无效
    }

    // 直接更新PWM占空比
    __HAL_TIM_SET_COMPARE(&PUSHER_MOTOR_PWM_TIMER, TIM_CHANNEL_1, duty);

    return 0;
}

/**
 * @brief 设置电机A方向
 * @param dir 方向（0或1）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t pusher_motor_set_motor_mp_a_dir(uint32_t dir)
{
    if (params_manager_set_motor_mp_a_dir(dir) != 0)
    {
        return 1; // 参数无效
    }

    // 更新GPIO引脚
    HAL_GPIO_WritePin(MOTOR_PM_A_DIRECTION_GPIO_Port, MOTOR_PM_A_DIRECTION_Pin,
                      (GPIO_PinState)dir);

    return params_manager_save();
}

/**
 * @brief 设置电机B方向
 * @param dir 方向（0或1）
 * @return 0: 成功, 1: 参数无效
 */
uint32_t pusher_motor_set_motor_mp_b_dir(uint32_t dir)
{
    if (params_manager_set_motor_mp_b_dir(dir) != 0)
    {
        return 1; // 参数无效
    }

    // 更新GPIO引脚
    HAL_GPIO_WritePin(MOTOR_PM_B_DIRECTION_GPIO_Port, MOTOR_PM_B_DIRECTION_Pin,
                      (GPIO_PinState)dir);

    return params_manager_save();
}

/**
 * @brief 获取运行时间
 * @return 运行时间（毫秒）
 */
uint32_t pusher_motor_get_direction_time(void)
{
    return params_manager_get_direction_time();
}

/**
 * @brief 获取等待时间
 * @return 等待时间（毫秒）
 */
uint32_t pusher_motor_get_wait_time(void)
{
    return params_manager_get_wait_time();
}

/**
 * @brief 获取PWM占空比
 * @return PWM占空比（0-500）
 */
uint32_t pusher_motor_get_pwm_duty(void)
{
    return params_manager_get_pwm_duty();
}

/**
 * @brief 获取最高转速
 * @return 最高转速（RPM）
 */
uint32_t pusher_motor_get_max_speed(void)
{
    return params_manager_get_max_speed();
}

/**
 * @brief 获取电机A方向
 * @return 方向（0或1）
 */
uint32_t pusher_motor_get_motor_mp_a_dir(void)
{
    return params_manager_get_motor_mp_a_dir();
}

/**
 * @brief 获取电机B方向
 * @return 方向（0或1）
 */
uint32_t pusher_motor_get_motor_mp_b_dir(void)
{
    return params_manager_get_motor_mp_b_dir();
}

/**
 * @brief 获取启动信号电平
 * @return 电平值（0或1）
 */
uint32_t pusher_motor_get_start_signal(void)
{
    return (uint32_t)HAL_GPIO_ReadPin(MOTOR_PM_ENABLE_GPIO_Port, MOTOR_PM_ENABLE_Pin);
}

/**
 * @brief 设置加速度值
 * @param accel 加速度值（0-50），0=无加速
 * @return 0: 成功, 1: 参数无效
 */
uint32_t pusher_motor_set_acceleration(uint8_t accel)
{
    if (accel > 50)
    {
        return 1; // 参数无效
    }
    acc_speed_value = accel;
    return 0;
}

/**
 * @brief 获取当前加速度值
 * @return 加速度值（0-50）
 */
uint8_t pusher_motor_get_acceleration(void)
{
    return acc_speed_value;
}
