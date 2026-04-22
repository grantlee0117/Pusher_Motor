# Pusher Motor 推料电机控制系统

基于 STM32F103C8T6 的推料电机控制固件，通过 PWM 实现电机正反转控制，支持串口命令行实时调参，参数可掉电保存。

## 功能特性

- **推料控制**：PWM 驱动电机正转推料 → 反转回位 → 自动停止
- **外部触发**：支持 PB7 引脚上升沿中断触发启动
- **串口 CLI**：通过 USART1 实时修改运行参数，波特率 115200
- **参数持久化**：方向运行时间、PWM 占空比等参数自动保存至 Flash
- **状态机管理**：IDLE → FORWARD → REVERSE → STOP 安全运转

## 硬件资源

| 功能 | 引脚 | 说明 |
|------|------|------|
| PWM 输出 | PB9 (TIM4_CH4) | 电机速度控制 |
| 方向控制 | PB8 | 电机转向控制 |
| 启动信号 | PB7 | 外部触发输入（上升沿） |
| 调试串口 | PA9 / PA10 | USART1 TX/RX |

- **MCU**: STM32F103C8T6 (ARM Cortex-M3, 72 MHz)
- **Flash**: 64 KB
- **RAM**: 20 KB

## 软件架构

```
┌─────────────────────────────────────┐
│           Application Layer          │
│  ┌─────────────┐  ┌───────────────┐ │
│  │    CLI      │  │ Pusher Motor  │ │
│  │   (cli)     │  │ (pusher_motor)│ │
│  └──────┬──────┘  └───────┬───────┘ │
└─────────┼─────────────────┼─────────┘
          │                 │
┌─────────┼─────────────────┼─────────┐
│    HAL Library (STM32Cube)          │
│  ┌─────────┐ ┌─────────┐ ┌───────┐ │
│  │  UART   │ │   TIM   │ │ GPIO  │ │
│  └─────────┘ └─────────┘ └───────┘ │
└─────────────────────────────────────┘
```

### 核心模块

- **`pusher_motor.c/h`** — 电机控制与状态机，支持从 Flash 加载/保存参数
- **`cli.c/h`** — 串口命令行解析与交互
- **`flash_storage.c/h`** — Flash 最后一页参数持久化存储（带校验和）
- **`stm32f1xx_it.c`** — 外部中断与系统中断处理

## 构建与烧录

### 环境要求

- [CMake](https://cmake.org/) >= 3.22
- [Ninja](https://ninja-build.org/)（推荐）或 Make
- [arm-none-eabi-gcc](https://developer.arm.com/downloads/-/gnu-rm) 工具链
- (可选) [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html) 或 OpenOCD 烧录

### 编译

```bash
# 创建构建目录
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# 编译
cmake --build build
```

编译完成后，固件位于 `build/Pusher_Motor.elf` 和 `build/Pusher_Motor.hex`。

### 烧录（示例）

使用 STM32CubeProgrammer：
```bash
STM32_Programmer_CLI -c port=SWD -w build/Pusher_Motor.hex -v -rst
```

或使用 OpenOCD：
```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
        -c "program build/Pusher_Motor.hex verify reset exit"
```

## 串口命令

连接 USART1（PA9/PA10），波特率 **115200**，数据位 8，停止位 1，无校验。

| 命令 | 功能 | 示例 |
|------|------|------|
| `start` | 启动推料电机 | `start` |
| `set direction_time <ms>` | 设置单方向运行时间（毫秒） | `set direction_time 1500` |
| `set pwm_duty <value>` | 设置 PWM 占空比（0–1000） | `set pwm_duty 500` |
| `get direction_time` | 查询当前方向运行时间 | `get direction_time` |
| `get pwm_duty` | 查询当前 PWM 占空比 | `get pwm_duty` |
| `help` | 显示帮助信息 | `help` |

> 参数修改后自动保存至 Flash，下次上电自动恢复。

## 项目结构

```
Pusher_Motor/
├── Core/
│   ├── Inc/                      # 头文件
│   │   ├── main.h
│   │   ├── pusher_motor.h        # 电机控制接口
│   │   ├── cli.h                 # CLI 接口
│   │   ├── flash_storage.h       # Flash 存储接口
│   │   ├── usart.h
│   │   ├── tim.h
│   │   └── gpio.h
│   └── Src/                      # 源文件
│       ├── main.c                # 主程序
│       ├── pusher_motor.c        # 电机控制实现
│       ├── cli.c                 # CLI 实现
│       ├── flash_storage.c       # Flash 存储实现
│       ├── stm32f1xx_it.c        # 中断处理
│       ├── usart.c
│       ├── tim.c
│       └── gpio.c
├── Drivers/
│   ├── CMSIS/                    # CMSIS 内核驱动
│   └── STM32F1xx_HAL_Driver/     # HAL 库
├── cmake/                        # CMake 工具链与配置
├── startup_stm32f103xb.s         # 启动文件
├── STM32F103XX_FLASH.ld          # 链接脚本
├── CMakeLists.txt                # 主构建配置
└── Pusher_Motor.ioc              # STM32CubeMX 配置文件
```

## 默认参数

| 参数 | 默认值 | 范围 |
|------|--------|------|
| 方向运行时间 | 1500 ms | > 0 |
| PWM 占空比 | 500 | 0 – 1000 |

参数在首次运行时从 Flash 加载，若 Flash 无有效数据则使用上述默认值。

## 注意事项

1. **中断优先级**：EXTI (PB7) 中断优先级应合理设置，避免影响主循环时序。
2. **主循环阻塞**：`pusher_motor_loop()` 与 `cli_process()` 需在主循环中持续快速调用，避免长时间阻塞。
3. **Flash 寿命**：参数修改会触发 Flash 擦写，频繁修改请注意 Flash 寿命（典型 10,000 次擦写）。
4. **PWM 频率**：由 TIM4 时钟分频与重装载值决定，默认通常为 1 kHz–20 kHz 范围，可按需调整。

## 调试建议

- **串口日志**：通过 USART1 观察 CLI 输出与命令响应。
- **逻辑分析仪**：监测 PB7（触发）、PB8（方向）、PB9（PWM）验证时序。
- **断点调试**：在 `pusher_motor_loop()` 状态转换处设置断点，观察状态机行为。
