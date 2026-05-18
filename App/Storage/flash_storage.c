#include "flash_storage.h"

// 默认参数值
#define DEFAULT_DIRECTION_TIME_MS 250 // 默认运行时间，单位：毫秒
#define DEFAULT_PWM_DUTY 250          // 默认PWM占空比，500最大

/**
 * @brief 计算校验和
 * @param data 参数结构体指针
 * @return 校验和
 */
static uint32_t CalculateChecksum(FlashStorage_t *data)
{
    return data->direction_time_ms + data->pwm_duty + data->wait_time_ms + data->max_speed_rpm + data->motor_mp_a_dir + data->motor_mp_b_dir;
}

/**
 * @brief 初始化 Flash 存储
 * @return 0: 成功, 1: 使用默认值
 */
uint32_t FlashStorage_Init(void)
{
    FlashStorage_t storage;

    // 尝试读取 Flash 数据
    if (FlashStorage_Read(&storage) == HAL_OK)
    {
        // 检查数据是否有效
        if (FlashStorage_IsValid(&storage))
        {
            return 0; // 成功读取有效数据
        }
    }

    // 数据无效或读取失败，使用默认值并写入 Flash
    storage.direction_time_ms = DEFAULT_DIRECTION_TIME_MS;
    storage.pwm_duty = DEFAULT_PWM_DUTY;
    storage.wait_time_ms = 250;   // 默认等待时间，单位：毫秒
    storage.max_speed_rpm = 3655; // 默认最高转速，单位：RPM
    storage.motor_mp_a_dir = 1;   // 默认值
    storage.motor_mp_b_dir = 0;   // 默认值
    storage.checksum = CalculateChecksum(&storage);

    FlashStorage_Write(&storage);

    return 1; // 使用了默认值
}

/**
 * @brief 从 Flash 读取参数
 * @param data 参数结构体指针
 * @return HAL_StatusTypeDef
 */
uint32_t FlashStorage_Read(FlashStorage_t *data)
{
    // 从 Flash 地址读取数据
    data->direction_time_ms = *(__IO uint32_t *)(FLASH_STORAGE_ADDR);
    data->pwm_duty = *(__IO uint32_t *)(FLASH_STORAGE_ADDR + 4);
    data->wait_time_ms = *(__IO uint32_t *)(FLASH_STORAGE_ADDR + 8);
    data->max_speed_rpm = *(__IO uint32_t *)(FLASH_STORAGE_ADDR + 12);
    data->motor_mp_a_dir = *(__IO uint32_t *)(FLASH_STORAGE_ADDR + 16);
    data->motor_mp_b_dir = *(__IO uint32_t *)(FLASH_STORAGE_ADDR + 20);
    data->checksum = *(__IO uint32_t *)(FLASH_STORAGE_ADDR + 24);

    return HAL_OK;
}

/**
 * @brief 写入参数到 Flash
 * @param data 参数结构体指针
 * @return HAL_StatusTypeDef
 */
uint32_t FlashStorage_Write(FlashStorage_t *data)
{
    HAL_StatusTypeDef status;
    uint32_t PageError;
    FLASH_EraseInitTypeDef EraseInitStruct;

    // 计算校验和
    data->checksum = CalculateChecksum(data);

    // 解锁 Flash
    HAL_FLASH_Unlock();

    // 擦除一页
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FLASH_STORAGE_ADDR;
    EraseInitStruct.NbPages = 1;
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return status;
    }

    // 写入数据
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_STORAGE_ADDR, data->direction_time_ms);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return status;
    }

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_STORAGE_ADDR + 4, data->pwm_duty);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return status;
    }

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_STORAGE_ADDR + 8, data->wait_time_ms);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return status;
    }

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_STORAGE_ADDR + 12, data->max_speed_rpm);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return status;
    }

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_STORAGE_ADDR + 16, data->motor_mp_a_dir);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return status;
    }

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_STORAGE_ADDR + 20, data->motor_mp_b_dir);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return status;
    }

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_STORAGE_ADDR + 24, data->checksum);

    // 锁定 Flash
    HAL_FLASH_Lock();

    return status;
}

/**
 * @brief 擦除 Flash 存储页
 * @return HAL_StatusTypeDef
 */
uint32_t FlashStorage_Erase(void)
{
    HAL_StatusTypeDef status;
    uint32_t PageError;
    FLASH_EraseInitTypeDef EraseInitStruct;

    // 解锁 Flash
    HAL_FLASH_Unlock();

    // 擦除一页
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FLASH_STORAGE_ADDR;
    EraseInitStruct.NbPages = 1;
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

    // 锁定 Flash
    HAL_FLASH_Lock();

    return status;
}

/**
 * @brief 检查数据是否有效
 * @param data 参数结构体指针
 * @return 1: 有效, 0: 无效
 */
uint32_t FlashStorage_IsValid(FlashStorage_t *data)
{
    // 检查参数范围
    if (data->direction_time_ms == 0xFFFFFFFF || data->pwm_duty == 0xFFFFFFFF ||
        data->wait_time_ms == 0xFFFFFFFF || data->max_speed_rpm == 0xFFFFFFFF ||
        data->motor_mp_a_dir == 0xFFFFFFFF || data->motor_mp_b_dir == 0xFFFFFFFF)
    {
        return 0; // Flash 未写入数据
    }

    if (data->direction_time_ms == 0 || data->direction_time_ms > 60000)
    {
        return 0; // 方向时间超出范围
    }

    if (data->pwm_duty > 500)
    {
        return 0; // PWM 占空比超出范围
    }

    if (data->wait_time_ms > 10000)
    {
        return 0; // 等待时间超出范围
    }

    if (data->max_speed_rpm == 0 || data->max_speed_rpm > 10000)
    {
        return 0; // 最高转速超出范围
    }

    if (data->motor_mp_a_dir != 0 && data->motor_mp_a_dir != 1)
    {
        return 0; // 电机A方向超出范围
    }

    if (data->motor_mp_b_dir != 0 && data->motor_mp_b_dir != 1)
    {
        return 0; // 电机B方向超出范围
    }

    // 检查校验和
    if (data->checksum != CalculateChecksum(data))
    {
        return 0; // 校验和错误
    }

    return 1; // 数据有效
}
