#ifndef __FLASH_STORAGE_H__
#define __FLASH_STORAGE_H__

#include "main.h"

// Flash 存储地址（使用最后一页，STM32F103C8T6 有 64KB Flash，每页 1KB）
#define FLASH_STORAGE_ADDR  0x0800FC00  // 最后一页起始地址

// 参数存储结构
typedef struct {
    uint32_t direction_time_ms;  // 方向运行时间
    uint32_t pwm_duty;           // PWM 占空比
    uint32_t checksum;           // 校验和
} FlashStorage_t;

// 函数声明
uint32_t FlashStorage_Init(void);
uint32_t FlashStorage_Read(FlashStorage_t *data);
uint32_t FlashStorage_Write(FlashStorage_t *data);
uint32_t FlashStorage_Erase(void);
uint32_t FlashStorage_IsValid(FlashStorage_t *data);

#endif /* __FLASH_STORAGE_H__ */
