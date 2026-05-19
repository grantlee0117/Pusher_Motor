# Core 目录

`Core/` 保留 STM32CubeMX 生成或维护的固件骨架。

- `Inc/`: `main.h`、外设头文件、HAL 配置和中断声明
- `Src/`: `main.c`、外设初始化、中断、系统文件和 syscall 文件
- `Startup/`: MCU 启动汇编文件

业务实现不要放进 `Core/Src`。如果必须修改 CubeMX 管辖文件，尽量只在 `USER CODE BEGIN/END` 区域内调用 `App/` 层函数。

## 修改约定

- 通过 CubeMX 修改外设、时钟或引脚时，以 `Pusher_Motor.ioc` 为入口。
- 重新生成代码后，需要重点检查 `Core/Src/main.c`、`gpio.c`、`tim.c`、`usart.c` 和 `stm32f1xx_hal_msp.c`。
- 手写业务逻辑不要直接沉到 `Core/`，应通过 `App/System` 进入应用层。
- 硬件资料发生变化时，不要只改 `Core/`；需要先确认 `hardware/` 和 `docs/` 中的接口说明。
