#ifndef __CLI_H__
#define __CLI_H__

#include "main.h"

// CLI 命令定义
typedef enum {
    CLI_CMD_NONE,               // 无命令
    CLI_CMD_DEVICE_MOTOR_PUSH,  // device motor push=time,wait,duty
    CLI_CMD_UNKNOWN             // 未知命令
} CliCommand_t;

// CLI 配置结构体
typedef struct {
    UART_HandleTypeDef *huart;  // 串口句柄
} CliConfig_t;

// 外部变量声明
extern CliConfig_t cli_config;

// 函数声明
void cli_init(UART_HandleTypeDef *huart);
void cli_process(void);

/* RX DMA 缓冲区（供 IDLE 中断使用） */
#define RX_DMA_BUF_SIZE 256
extern uint8_t rx_dma_buffer[RX_DMA_BUF_SIZE];

/* RX DMA 数据处理（在 USART IDLE 中断中调用） */
void cli_process_rx_dma(uint8_t *data, uint16_t len);

/* TX DMA 完成处理（在 HAL_UART_TxCpltCallback 中调用） */
void cli_tx_dma_complete(void);

#endif /* __CLI_H__ */
