#include "pusher_motor.h"
#include "main.h"
#include "flash_storage.h"


extern TIM_HandleTypeDef htim4;
// 定义推料电机的定时器和通道
#define PUSHER_MOTOR_PWM_TIMER htim4
#define PUSHER_MOTOR_PWM_CHANNEL TIM_CHANNEL_4

// 推料电机状态定义
typedef enum {
    MOTOR_STATE_IDLE = 0,      // 空闲状态
    MOTOR_STATE_RUNNING,        // 运行状态
    MOTOR_STATE_STOP            // 停止状态
} MotorState_t;

// 推料电机控制变量
MotorState_t motor_state = MOTOR_STATE_IDLE;
uint32_t motor_start_time = 0;

// 推料电机参数（全局变量，可通过CLI修改，存储在Flash中）
uint32_t MOTOR_DIRECTION_TIME_MS = 2500;  // 运行时间（毫秒）
uint32_t MOTOR_PWM_DUTY = 250;            // PWM占空比（0-500）
uint32_t MOTOR_PWM_DUTY_MAX = 500;            // PWM最大占空比

// 开始工作标志位
volatile uint8_t pusher_motor_start_flag = 0;
// 工作标志位
uint8_t pusher_motor_work_flag = 0;



void pusher_motor_init(void)
{
    // 初始化PWM输出
    HAL_TIM_PWM_Start(&PUSHER_MOTOR_PWM_TIMER, PUSHER_MOTOR_PWM_CHANNEL); 
    __HAL_TIM_SetCompare(&PUSHER_MOTOR_PWM_TIMER, PUSHER_MOTOR_PWM_CHANNEL, MOTOR_PWM_DUTY_MAX);  

    
    FlashStorage_t storage;
    
    // 从Flash读取参数
    if (FlashStorage_Read(&storage) == HAL_OK && FlashStorage_IsValid(&storage))
    {
        MOTOR_DIRECTION_TIME_MS = storage.direction_time_ms;
        MOTOR_PWM_DUTY = storage.pwm_duty;
    }
    else
    {
        // Flash数据无效，使用默认值并写入Flash
        storage.direction_time_ms = MOTOR_DIRECTION_TIME_MS;
        storage.pwm_duty = MOTOR_PWM_DUTY;
        FlashStorage_Write(&storage);
    }
    
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
                // 启动PWM输出
                __HAL_TIM_SetCompare(&PUSHER_MOTOR_PWM_TIMER, PUSHER_MOTOR_PWM_CHANNEL, MOTOR_PWM_DUTY);
                // 设置工作标志
                pusher_motor_work_flag = 1;
                // 记录启动时间
                motor_start_time = current_time;
                // 进入运行状态
                motor_state = MOTOR_STATE_RUNNING;
            }
            break;
            
        case MOTOR_STATE_RUNNING:
            // 检查运行时间
            if ((current_time - motor_start_time) >= MOTOR_DIRECTION_TIME_MS)
            {
                // 运行结束，进入停止状态
                motor_state = MOTOR_STATE_STOP;
            }
            break;
            
        case MOTOR_STATE_STOP:
            // 电机停止
            __HAL_TIM_SetCompare(&PUSHER_MOTOR_PWM_TIMER, PUSHER_MOTOR_PWM_CHANNEL, MOTOR_PWM_DUTY_MAX);
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
    FlashStorage_t storage;
    storage.direction_time_ms = MOTOR_DIRECTION_TIME_MS;
    storage.pwm_duty = MOTOR_PWM_DUTY;
    return FlashStorage_Write(&storage);
}
