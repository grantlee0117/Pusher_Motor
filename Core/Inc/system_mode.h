#ifndef __SYSTEM_MODE_H__
#define __SYSTEM_MODE_H__

#include <stdint.h>

typedef enum {
    SYSTEM_MODE_BOOT = 0,     // 系统初始化中
    SYSTEM_MODE_IDLE,         // 空闲，允许配置和启动
    SYSTEM_MODE_RUNNING,      // 正在执行一次动作
    SYSTEM_MODE_SERVICE,      // 售后调试模式，允许手动PWM
    SYSTEM_MODE_ERROR         // 错误保护模式
} SystemMode_t;

void system_mode_init(void);
SystemMode_t system_mode_get(void);
uint32_t system_mode_set(SystemMode_t mode);
const char *system_mode_to_string(SystemMode_t mode);

uint8_t system_mode_can_configure(void);
uint8_t system_mode_can_start(void);
uint8_t system_mode_can_direct_pwm(void);

uint32_t system_mode_enter_service(void);
uint32_t system_mode_exit_service(void);

#endif /* __SYSTEM_MODE_H__ */
