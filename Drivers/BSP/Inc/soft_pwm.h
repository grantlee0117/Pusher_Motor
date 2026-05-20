#ifndef __SOFT_PWM_H__
#define __SOFT_PWM_H__

#include <stdint.h>

void soft_pwm_init(void);
void E1_Set_Duty(uint16_t duty);
void E2_Set_Duty(uint16_t duty);

#endif /* __SOFT_PWM_H__ */
