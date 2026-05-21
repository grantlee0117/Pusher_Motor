#include "config.h"
#include "soft_pwm.h"
#include "tim.h"

volatile uint16_t e1_pwm_duty = 0;
volatile uint16_t e2_pwm_duty = 0;
volatile uint16_t pwm_cnt = 0;

void soft_pwm_init(void)
{
    HAL_TIM_Base_Start_IT(&htim1);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        /* E1 - 直接操作 BSRR 寄存器，减少 HAL 开销 */
        if (pwm_cnt < e1_pwm_duty)
        {
            MOTOR_E1_SPEED_PORT->BSRR = (uint32_t)MOTOR_E1_SPEED_PIN << 16U; // 低电平 = 运转
        }
        else
        {
            MOTOR_E1_SPEED_PORT->BSRR = MOTOR_E1_SPEED_PIN;                  // 高电平 = 停止
        }

        /* E2 - 直接操作 BSRR 寄存器，减少 HAL 开销 */
        if (pwm_cnt < e2_pwm_duty)
        {
            MOTOR_E2_SPEED_PORT->BSRR = (uint32_t)MOTOR_E2_SPEED_PIN << 16U; // 低电平 = 运转
        }
        else
        {
            MOTOR_E2_SPEED_PORT->BSRR = MOTOR_E2_SPEED_PIN;                  // 高电平 = 停止
        }

        pwm_cnt++;
        if (pwm_cnt >= SOFT_PWM_PERIOD)
        {
            pwm_cnt = 0;
        }
    }
}

void E1_Set_Duty(uint16_t duty)
{
    if (duty > PUSHER_MOTOR_MAX_DUTY)
    {
        duty = PUSHER_MOTOR_MAX_DUTY;
    }
    e1_pwm_duty = (uint16_t)((uint32_t)(PUSHER_MOTOR_MAX_DUTY - duty) * SOFT_PWM_PERIOD / PUSHER_MOTOR_MAX_DUTY);
}

void E2_Set_Duty(uint16_t duty)
{
    if (duty > PUSHER_MOTOR_MAX_DUTY)
    {
        duty = PUSHER_MOTOR_MAX_DUTY;
    }
    e2_pwm_duty = (uint16_t)((uint32_t)(PUSHER_MOTOR_MAX_DUTY - duty) * SOFT_PWM_PERIOD / PUSHER_MOTOR_MAX_DUTY);
}
