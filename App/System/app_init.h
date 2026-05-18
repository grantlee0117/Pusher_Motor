#ifndef APP_INIT_H
#define APP_INIT_H

#include "main.h"

void app_system_init(UART_HandleTypeDef *cli_uart);
void app_system_loop(void);

#endif /* APP_INIT_H */
