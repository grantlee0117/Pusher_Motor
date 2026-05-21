#ifndef __SOFT_PWM_H__
#define __SOFT_PWM_H__

#include <stdint.h>

#define SOFT_PWM_PERIOD 100

extern volatile uint16_t pwm_cnt;
extern volatile uint16_t e1_pwm_duty;
extern volatile uint16_t e2_pwm_duty;

void soft_pwm_init(void);
void E1_Set_Duty(uint16_t duty);
void E2_Set_Duty(uint16_t duty);

#endif /* __SOFT_PWM_H__ */
