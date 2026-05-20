/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MOTOR_A2_DIR_Pin GPIO_PIN_13
#define MOTOR_A2_DIR_GPIO_Port GPIOC
#define PUSH_START_Pin GPIO_PIN_0
#define PUSH_START_GPIO_Port GPIOA
#define PUSH_START_EXTI_IRQn EXTI0_IRQn
#define MOTOR_A1_SPEED_Pin GPIO_PIN_1
#define MOTOR_A1_SPEED_GPIO_Port GPIOA
#define MOTOR_A1_DIR_Pin GPIO_PIN_2
#define MOTOR_A1_DIR_GPIO_Port GPIOA
#define MOTOR_A1_FEEDBACK_Pin GPIO_PIN_3
#define MOTOR_A1_FEEDBACK_GPIO_Port GPIOA
#define MOTOR_B1_STOP_Pin GPIO_PIN_4
#define MOTOR_B1_STOP_GPIO_Port GPIOA
#define MOTOR_B2_STOP_Pin GPIO_PIN_5
#define MOTOR_B2_STOP_GPIO_Port GPIOA
#define MOTOR_B1_SPEED_Pin GPIO_PIN_6
#define MOTOR_B1_SPEED_GPIO_Port GPIOA
#define MOTOR_B2_SPEED_Pin GPIO_PIN_7
#define MOTOR_B2_SPEED_GPIO_Port GPIOA
#define MOTOR_B1_DIR_Pin GPIO_PIN_0
#define MOTOR_B1_DIR_GPIO_Port GPIOB
#define MOTOR_B2_DIR_Pin GPIO_PIN_1
#define MOTOR_B2_DIR_GPIO_Port GPIOB
#define MOTOR_E2_DIR_Pin GPIO_PIN_15
#define MOTOR_E2_DIR_GPIO_Port GPIOB
#define MOTOR_E1_DIR_Pin GPIO_PIN_11
#define MOTOR_E1_DIR_GPIO_Port GPIOA
#define MOTOR_E1_SPEED_Pin GPIO_PIN_12
#define MOTOR_E1_SPEED_GPIO_Port GPIOA
#define SIGNAL1_Pin GPIO_PIN_3
#define SIGNAL1_GPIO_Port GPIOB
#define SIGNAL2_Pin GPIO_PIN_4
#define SIGNAL2_GPIO_Port GPIOB
#define MOTOR_E2_SPEED_Pin GPIO_PIN_7
#define MOTOR_E2_SPEED_GPIO_Port GPIOB
#define MOTOR_A2_SPEED_Pin GPIO_PIN_8
#define MOTOR_A2_SPEED_GPIO_Port GPIOB
#define MOTOR_A2_FEEDBACK_Pin GPIO_PIN_9
#define MOTOR_A2_FEEDBACK_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
