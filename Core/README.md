# Core 目录

`Core/` 保留 STM32CubeMX 生成或维护的固件骨架。

- `Inc/`: `main.h`、外设头文件、HAL 配置和中断声明
- `Src/`: `main.c`、外设初始化、中断、系统文件和 syscall 文件
- `Startup/`: MCU 启动汇编文件

业务实现不要放进 `Core/Src`。如果必须修改 CubeMX 管辖文件，尽量只在 `USER CODE BEGIN/END` 区域内调用 `App/` 层函数。
