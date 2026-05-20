#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "main.h"

#define PUSHER_MOTOR_MAX_DUTY 500

#define PUSH_START_PIN PUSH_START_Pin
#define PUSH_START_PORT PUSH_START_GPIO_Port

/* ==========================================================
 * 电机E1/E2 驱动配置
 * ========================================================== */

/* 电机E1驱动配置 */
#define MOTOR_E1_DIR_PIN MOTOR_E1_DIR_Pin
#define MOTOR_E1_DIR_PORT MOTOR_E1_DIR_GPIO_Port
#define MOTOR_E1_SPEED_PIN MOTOR_E1_SPEED_Pin
#define MOTOR_E1_SPEED_PORT MOTOR_E1_SPEED_GPIO_Port

/* 电机E2驱动配置 */
#define MOTOR_E2_DIR_PIN MOTOR_E2_DIR_Pin
#define MOTOR_E2_DIR_PORT MOTOR_E2_DIR_GPIO_Port
#define MOTOR_E2_SPEED_PIN MOTOR_E2_SPEED_Pin
#define MOTOR_E2_SPEED_PORT MOTOR_E2_SPEED_GPIO_Port
/* ==========================================================
 * 电机B1/B2 驱动配置
 * ========================================================== */

/* ==========================================================
 * 电机A1/A2 驱动配置
 * ========================================================== */

#endif /* __CONFIG_H__ */
