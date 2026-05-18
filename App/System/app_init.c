#include "app_init.h"

#include "cli.h"
#include "pusher_motor.h"

void app_system_init(UART_HandleTypeDef *cli_uart)
{
    pusher_motor_init();
    cli_init(cli_uart);
}

void app_system_loop(void)
{
    cli_process();
    pusher_motor_loop();
}
