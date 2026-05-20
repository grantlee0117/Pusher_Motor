#include "config.h"
#include "soft_pwm.h"
#include "tim.h"

/* 软件 PWM 分辨率（与 TIM1 Period 匹配：200kHz / 200 = 1kHz PWM） */
#define SOFT_PWM_PERIOD 100

/* 软件 PWM 占空比 0~SOFT_PWM_PERIOD */
volatile uint16_t e1_pwm_duty = 0;
volatile uint16_t e2_pwm_duty = 0;

/* 计数器 */
volatile uint16_t pwm_cnt = 0;

/**
 * @brief 软件 PWM 初始化
 * @details 启动 TIM1 中断，用于两路软件 PWM 输出
 */
void soft_pwm_init(void)
{
    /* 启动 TIM1 基础定时器中断 */
    HAL_TIM_Base_Start_IT(&htim1);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        /* E1 */
        if (pwm_cnt < e1_pwm_duty)
        {
            HAL_GPIO_WritePin(MOTOR_E1_SPEED_PORT, MOTOR_E1_SPEED_PIN, GPIO_PIN_RESET); // 低电平 = 运转
        }
        else
        {
            HAL_GPIO_WritePin(MOTOR_E1_SPEED_PORT, MOTOR_E1_SPEED_PIN, GPIO_PIN_SET); // 高电平 = 停止
        }

        /* E2 */
        if (pwm_cnt < e2_pwm_duty)
        {
            HAL_GPIO_WritePin(MOTOR_E2_SPEED_PORT, MOTOR_E2_SPEED_PIN, GPIO_PIN_RESET); // 低电平 = 运转
        }
        else
        {
            HAL_GPIO_WritePin(MOTOR_E2_SPEED_PORT, MOTOR_E2_SPEED_PIN, GPIO_PIN_SET); // 高电平 = 停止
        }

        pwm_cnt++;
        if (pwm_cnt >= SOFT_PWM_PERIOD)
        {
            pwm_cnt = 0;
        }
    }
}

/**
 * @brief 设置 E1 电机 PWM 占空比
 * @param duty 占空比（0~PUSHER_MOTOR_MAX_DUTY），500=停止，0=全速
 */
void E1_Set_Duty(uint16_t duty)
{
    if (duty > PUSHER_MOTOR_MAX_DUTY)
    {
        duty = PUSHER_MOTOR_MAX_DUTY;
    }
    e1_pwm_duty = (uint16_t)((uint32_t)(PUSHER_MOTOR_MAX_DUTY - duty) * SOFT_PWM_PERIOD / PUSHER_MOTOR_MAX_DUTY);
}

/**
 * @brief 设置 E2 电机 PWM 占空比
 * @param duty 占空比（0~PUSHER_MOTOR_MAX_DUTY），500=停止，0=全速
 */
void E2_Set_Duty(uint16_t duty)
{
    if (duty > PUSHER_MOTOR_MAX_DUTY)
    {
        duty = PUSHER_MOTOR_MAX_DUTY;
    }
    e2_pwm_duty = (uint16_t)((uint32_t)(PUSHER_MOTOR_MAX_DUTY - duty) * SOFT_PWM_PERIOD / PUSHER_MOTOR_MAX_DUTY);
}
