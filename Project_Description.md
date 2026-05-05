# Pusher Motor 推料电机项目说明

## 项目概述

本项目是一个基于 STM32F103C8T6 微控制器的推料电机控制系统，通过 PWM 控制电机运行，实现推料功能。项目包含硬件控制逻辑和串口命令行接口（CLI），支持通过串口实时调整运行参数，参数自动保存到 Flash，断电后不丢失。

## 系统架构

### 硬件平台
- **MCU**: STM32F103C8T6 (Cortex-M3, 72MHz)
- **PWM 输出**: TIM4_CH4 (PB9)
- **方向控制 A**: GPIO 输出 (PB8) — MOTOR_PM_A_DIRECTION
- **方向控制 B**: GPIO 输出 (PB7) — MOTOR_PM_B_DIRECTION
- **启动信号**: GPIO 输入中断 (PB6) — MOTOR_PM_ENABLE，上升沿触发
- **调试串口**: USART1 (PA9/PA10)

### 硬件连接示意图

![硬件接线图](.nn/materials/2.png)

> 图中展示了主控板、电机驱动板、电机模块之间的实际接线。注意：图片标注的"推料时间暂定3S"为早期设定，实际固件默认值已更新为 1500ms，可通过 CLI 命令 `set direction_time` 调整。

### 软件架构

```
┌─────────────────────────────────────────┐
│              应用层 (Application)        │
│  ┌─────────────┐    ┌───────────────┐  │
│  │  CLI 模块   │    │ 推料电机控制   │  │
│  │  (cli.c)    │    │(pusher_motor.c)│ │
│  └──────┬──────┘    └───────┬───────┘  │
│         │                   │          │
│  ┌──────┴───────────────────┴──────┐   │
│  │      参数管理 (params_manager.c) │   │
│  └──────────────┬──────────────────┘   │
└─────────────────┼──────────────────────┘
                  │
┌─────────────────┼──────────────────────┐
│                 ▼                      │
│      HAL 库 (Hardware Abstraction Layer)│
│  ┌─────────┐  ┌─────────┐  ┌─────────┐ │
│  │  UART   │  │   TIM   │  │  GPIO   │ │
│  └─────────┘  └─────────┘  └─────────┘ │
└────────────────────────────────────────┘
```

## 核心模块说明

### 1. 推料电机控制模块 (pusher_motor.c)

#### 功能
- 初始化 PWM 输出和 GPIO 方向控制
- 实现推料动作控制（启动 -> 等待 -> 运行 -> 停止）
- 状态机管理电机运行状态
- 提供速度计算和占空比转换函数

#### 关键参数
```c
#define DEFAULT_DIRECTION_TIME_MS   1500  // 默认运行时间（毫秒）
#define DEFAULT_PWM_DUTY            30    // 默认PWM占空比（0-500）
#define DEFAULT_WAIT_TIME_MS        250   // 默认等待时间（毫秒）
#define DEFAULT_MAX_SPEED_RPM       3655  // 默认最高转速（RPM）
#define DEFAULT_MOTOR_MP_A_DIR      1     // 默认电机A方向（高电平）
#define DEFAULT_MOTOR_MP_B_DIR      0     // 默认电机B方向（低电平）
```

#### 状态机设计
```
┌─────────┐    start     ┌───────────┐
│  IDLE   │ ───────────> │   WAIT    │
│ (空闲)  │              │  (等待)   │
└─────────┘              └─────┬─────┘
     ▲                         │ 等待时间到
     │                         ▼
     │                   ┌───────────┐
     │                   │  RUNNING  │
     │                   │  (运行)   │
     │                   └─────┬─────┘
     │                         │ 运行时间到
     │                         ▼
     │                   ┌───────────┐
     └────────────────── │   STOP    │
                         │  (停止)   │
                         └───────────┘
```

#### 工作流程
1. **触发启动**：外部中断（PB6 上升沿）或 CLI 命令触发 `pusher_motor_start()`
2. **等待阶段**：进入等待状态，延时 `wait_time_ms`
3. **运行阶段**：启动 PWM 输出，按照设定的占空比运行 `direction_time_ms`
4. **停止阶段**：停止 PWM 输出（占空比设为 500），回到空闲状态

### 2. CLI 命令行接口模块 (cli.c)

#### 功能
- 串口命令接收和解析
- 实时修改电机运行参数
- 命令回显和响应输出

#### 接收机制
```
串口数据 → 环形缓冲区 → 命令解析 → 执行处理 → 响应输出
```

#### 环形缓冲区设计
```c
#define RX_BUF_SIZE 512
static uint8_t rx_ring_buf[RX_BUF_SIZE];  // 接收环形缓冲区
static volatile uint16_t rx_head = 0;     // 写指针
static volatile uint16_t rx_tail = 0;     // 读指针
```

#### 支持的命令
| 命令 | 功能 |
|------|------|
| `start` | 启动推料电机 |
| `set direction_time <ms>` | 设置运行时间（1-60000 ms） |
| `set pwm_duty <value>` | 设置 PWM 占空比（0-500） |
| `set wait_time <ms>` | 设置等待时间（0-10000 ms） |
| `set max_speed <rpm>` | 设置最高转速（1-10000 RPM） |
| `set speed <cm/min>` | 设置速度（0+ cm/min） |
| `set motor_mp_a_dir <0\|1>` | 设置电机A方向 |
| `set motor_mp_b_dir <0\|1>` | 设置电机B方向 |
| `set new_pwm_duty <value>` | 直接设置PWM占空比（不保存Flash） |
| `get direction_time` | 获取当前运行时间 |
| `get pwm_duty` | 获取当前 PWM 占空比 |
| `get wait_time` | 获取当前等待时间 |
| `get max_speed` | 获取当前最高转速 |
| `get speed` | 获取当前速度 |
| `get motor_mp_a_dir` | 获取电机A方向 |
| `get motor_mp_b_dir` | 获取电机B方向 |
| `get start_signal` | 获取启动信号电平（PB6） |
| `help` / `?` | 显示帮助信息 |

### 3. 参数管理模块 (params_manager.c)

#### 功能
- 统一管理电机运行参数
- 参数有效性检查
- 参数持久化到 Flash

#### 参数结构体
```c
typedef struct {
    uint32_t direction_time_ms;  // 运行时间（毫秒）
    uint32_t pwm_duty;            // PWM占空比（0-500）
    uint32_t pwm_duty_max;        // PWM最大占空比（500）
    uint32_t wait_time_ms;         // 等待时间（毫秒）
    uint32_t max_speed_rpm;       // 最高转速（RPM）
    uint32_t motor_mp_a_dir;      // 电机A方向
    uint32_t motor_mp_b_dir;      // 电机B方向
} MotorParams_t;
```

### 4. 中断处理模块 (stm32f1xx_it.c)

#### 外部中断 EXTI9_5
- **触发源**：PB6 引脚（推料电机启动信号）
- **触发方式**：上升沿触发
- **处理逻辑**：调用 `pusher_motor_start()` 启动推料

## 数据流

### 推料触发流程
```
外部信号(PB6) ──> EXTI 中断 ──> pusher_motor_start()
                                          │
                                          ▼
                              ┌─────────────────────┐
                              │   pusher_motor_loop │
                              │   (主循环调用)      │
                              └─────────────────────┘
```

### CLI 命令流程
```
串口输入 ──> 环形缓冲区 ──> 命令解析 ──> 参数修改/电机控制
                │                           │
                ▼                           ▼
           字符回显                    响应输出
```

## 主循环逻辑

```c
int main(void)
{
    // 初始化
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM4_Init();          // PWM 定时器
    MX_USART1_UART_Init();   // 调试串口
    
    pusher_motor_init();     // 初始化推料电机（含参数管理器）
    cli_init(&huart1);       // 初始化 CLI
    
    while (1)
    {
        // 处理 CLI 命令
        cli_process();
        
        // 处理推料电机状态机
        pusher_motor_loop();
    }
}
```

## 关键时序

### PWM 输出
- **频率**：由 TIM4 配置决定（配置的是2kHz）
- **占空比**：0-500 对应实际控制值
  - 注意：PWM 逻辑为电平越高速度越慢，占空比 500 时电机停止，占空比 0 时速度最快
- **输出引脚**：PB9 (TIM4_CH4)

### 推料周期
```
时间(ms)
  0    ┌───────────┐
       │   等待    │  wait_time_ms
       │           ├───────────┐
       │           │   运行    │  direction_time_ms
  X    │           │           ├──────┐
       │           │           │ 停止 │
       └───────────┴───────────┘      │
                                      ▼
                                   回到 IDLE
```

## 配置参数

### 编译配置
- **构建工具**：CMake + Make
- **编译器**：arm-none-eabi-gcc
- **优化级别**：-O0 (Debug)

### 内存使用
- **RAM**：约 2.6 KB
- **FLASH**：约 17.5 KB

## 扩展接口

### 添加新命令
在 `cli.c` 中添加：
1. 在 `CliCommand_t` 枚举中添加新命令类型
2. 在 `parse_command()` 中添加命令解析逻辑
3. 在 `cli_execute_command()` 中添加命令执行代码

### 修改默认参数
在 `params_manager.c` 中修改：
```c
#define DEFAULT_DIRECTION_TIME_MS   1500  // 修改默认值
#define DEFAULT_PWM_DUTY            30    // 修改默认值
#define DEFAULT_WAIT_TIME_MS        250   // 修改默认值
#define DEFAULT_MAX_SPEED_RPM       3655  // 修改默认值
```

## 调试方法

1. **串口调试**：连接 USART1，使用 115200 波特率
2. **逻辑分析仪**：监测 PB6（触发）、PB7（B方向）、PB8（A方向）、PB9（PWM）
3. **断点调试**：在关键状态转换处设置断点

## 注意事项

1. **中断优先级**：确保 EXTI 中断优先级合理，不影响主循环
2. **缓冲区溢出**：环形缓冲区满时会丢弃新数据
3. **参数有效性**：CLI 会检查参数范围，无效参数会返回错误
4. **实时性**：主循环需要快速执行，避免阻塞
5. **PWM 逻辑**：电平越高速度越慢，占空比 500 为停止状态

## 文件结构

```
Pusher_Motor/
├── Core/
│   ├── Inc/
│   │   ├── main.h
│   │   ├── cli.h           # CLI 接口定义
│   │   ├── pusher_motor.h  # 推料电机接口定义
│   │   ├── params_manager.h # 参数管理器接口定义
│   │   └── flash_storage.h # Flash 存储接口定义
│   └── Src/
│       ├── main.c          # 主程序入口
│       ├── cli.c           # CLI 实现
│       ├── pusher_motor.c  # 推料电机控制实现
│       ├── params_manager.c # 参数管理器实现
│       ├── flash_storage.c # Flash 存储实现
│       ├── stm32f1xx_it.c  # 中断处理
│       └── usart.c         # 串口驱动
├── cmake/
│   └── stm32cubemx/
│       └── CMakeLists.txt  # 构建配置
└── Project_Description.md  # 本文档
```
