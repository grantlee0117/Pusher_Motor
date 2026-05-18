# 推料电机控制系统架构总览

## 系统概述

推料电机控制系统是一个基于 STM32F103C8T6 微控制器的嵌入式系统，用于控制推料电机的运行状态、速度和方向。系统采用状态机设计，支持通过串口进行调试和参数配置，实现了完整的电机控制功能。参数通过 Flash 持久化保存，断电后不丢失。

## 系统架构

### 1. 核心模块

#### 1.1 电机控制模块 (`pusher_motor.c`)

- **功能**：实现电机的初始化、启动、停止和状态管理
- **核心逻辑**：状态机设计，包含空闲、等待、运行和停止四个状态
- **关键函数**：
  - `pusher_motor_init()`：初始化电机控制，调用参数管理器加载参数，配置PWM输出
  - `pusher_motor_start()`：设置启动标志，触发状态机转换
  - `pusher_motor_loop()`：处理电机状态转换，是系统的核心运行函数
  - `pusher_motor_save_params()`：保存参数到Flash（通过参数管理器）
  - `pusher_motor_calculate_duty_from_speed()`：根据速度计算PWM占空比
  - `pusher_motor_calculate_speed_from_duty()`：根据占空比计算速度
  - `pusher_motor_set_max_speed()`：设置最高转速
  - `pusher_motor_set_speed()`：设置速度并计算对应占空比
  - `pusher_motor_set_pwm_duty_direct()`：直接设置PWM占空比（不保存Flash）

#### 1.2 串口调试模块 (`cli.c`)

- **功能**：提供命令行接口，支持通过串口控制和调试系统
- **通信模式**：轮询模式，非阻塞式处理，使用环形缓冲区
- **核心函数**：
  - `cli_init()`：初始化CLI模块，设置串口句柄
  - `cli_process()`：处理串口数据，解析命令并执行
  - `parse_command()`：解析命令字符串
  - `cli_execute_command()`：执行解析后的命令
  - `cli_send_string()`：发送字符串到串口（阻塞方式）
- **支持命令**：
  - `start`：启动推料电机
  - `set direction_time <ms>`：设置运行时间（1-60000 ms）
  - `set pwm_duty <value>`：设置PWM占空比（0-500）
  - `set wait_time <ms>`：设置等待时间（0-10000 ms）
  - `set max_speed <rpm>`：设置最高转速（1-10000 RPM）
  - `set speed <cm/min>`：设置速度（0+ cm/min）
  - `set motor_mp_a_dir <0|1>`：设置电机A方向
  - `set motor_mp_b_dir <0|1>`：设置电机B方向
  - `set new_pwm_duty <value>`：直接设置PWM占空比（不保存Flash）
  - `get direction_time`：获取运行时间
  - `get pwm_duty`：获取PWM占空比
  - `get wait_time`：获取等待时间
  - `get max_speed`：获取最高转速
  - `get speed`：获取当前速度
  - `get motor_mp_a_dir`：获取电机A方向
  - `get motor_mp_b_dir`：获取电机B方向
  - `get start_signal`：获取启动信号电平（PB6）
  - `help` / `?`：显示帮助信息

#### 1.3 参数管理模块 (`params_manager.c`)

- **功能**：统一管理电机运行参数，提供参数有效性检查和持久化接口
- **核心函数**：
  - `params_manager_init()`：初始化参数管理器，从Flash加载参数
  - `params_manager_get_all()`：获取所有参数
  - `params_manager_set_direction_time()` / `get`：运行时间读写
  - `params_manager_set_pwm_duty()` / `get`：PWM占空比读写
  - `params_manager_set_wait_time()` / `get`：等待时间读写
  - `params_manager_set_max_speed()` / `get`：最高转速读写
  - `params_manager_set_motor_mp_a_dir()` / `get`：电机A方向读写
  - `params_manager_set_motor_mp_b_dir()` / `get`：电机B方向读写
  - `params_manager_save()`：保存参数到Flash
  - `params_manager_load()`：从Flash加载参数

#### 1.4 Flash存储模块 (`flash_storage.c`)

- **功能**：将系统参数永久保存到Flash存储器
- **存储地址**：`0x0800FC00`（STM32F103C8T6的最后一页）
- **核心函数**：
  - `FlashStorage_Init()`：初始化Flash存储，检查数据有效性
  - `FlashStorage_Read()`：从Flash读取参数
  - `FlashStorage_Write()`：写入参数到Flash
  - `FlashStorage_IsValid()`：检查Flash数据有效性
  - `CalculateChecksum()`：计算校验和确保数据完整性
- **存储参数**：
  - 运行时间 (`direction_time_ms`)
  - PWM占空比 (`pwm_duty`)
  - 等待时间 (`wait_time_ms`)
  - 最高转速 (`max_speed_rpm`)
  - 电机A方向 (`motor_mp_a_dir`)
  - 电机B方向 (`motor_mp_b_dir`)

### 2. 硬件接口

#### 2.1 电机控制

- **PWM输出**：TIM4_CH4 (PB9)，用于控制电机速度
- **方向控制**：两个GPIO引脚
  - `MOTOR_PM_A_DIRECTION` (PB8)
  - `MOTOR_PM_B_DIRECTION` (PB7)
- **使能信号**：外部中断触发 (`MOTOR_PM_ENABLE` / PB6)，上升沿触发，用于启动电机

#### 2.2 串口通信

- **USART1** (PA9/PA10)：用于与上位机通信，波特率115200，8位数据位，1位停止位，无校验位

### 3. 状态机设计

#### 3.1 状态定义

- `MOTOR_STATE_IDLE`：空闲状态，等待启动信号
- `MOTOR_STATE_WAIT`：等待状态，等待设定的延迟时间
- `MOTOR_STATE_RUNNING`：运行状态，电机正常运行
- `MOTOR_STATE_STOP`：停止状态，电机停止运行

#### 3.2 状态转换

1. **空闲状态 → 等待状态**
   - 触发条件：收到启动信号（外部中断或串口命令）
   - 转换动作：清除启动标志，记录等待开始时间，进入等待状态

2. **等待状态 → 运行状态**
   - 触发条件：等待时间达到设定值
   - 转换动作：启动PWM输出，设置工作标志，记录运行开始时间，进入运行状态

3. **运行状态 → 停止状态**
   - 触发条件：运行时间达到设定值
   - 转换动作：进入停止状态

4. **停止状态 → 空闲状态**
   - 触发条件：进入停止状态后立即转换
   - 转换动作：停止PWM输出（占空比恢复500），清除工作标志，回到空闲状态

### 4. 速度控制算法

#### 4.1 计算原理

- **电机周长**：`π * 直径 = 3.14159 * 6 ≈ 18.8496 cm`
- **转速计算**：`转速 = 速度 / 周长`
- **占空比计算**：`占空比 = (最高转速 - 当前转速) / 最高转速 * 最大占空比`
  - 注意：占空比为 0 时速度最快，500 时停止

#### 4.2 误差分析

- **误差来源**：线性计算模型与实际电机特性可能存在偏差
- **优化方向**：进行实际测试，校准速度计算公式

### 5. 系统工作流程

#### 5.1 初始化流程

1. **系统启动**：STM32F103C8T6复位，执行启动代码
2. **硬件初始化**：初始化时钟、GPIO、定时器、串口等
3. **电机控制初始化**：调用 `pusher_motor_init()`，初始化参数管理器（从Flash加载参数），配置PWM输出
4. **CLI初始化**：调用 `cli_init()`，设置串口通信
5. **进入主循环**：系统进入空闲状态，等待触发

#### 5.2 运行流程

1. **主循环**：
   - 调用 `cli_process()`：处理串口数据，解析并执行命令
   - 调用 `pusher_motor_loop()`：处理电机状态转换

2. **启动电机**（外部中断触发）：
   - 外部信号 (PB6 上升沿) 触发 `EXTI9_5_IRQHandler()` 中断
   - 中断函数调用 `pusher_motor_start()`
   - `pusher_motor_start()` 设置 `pusher_motor_start_flag = 1`
   - 主循环中 `pusher_motor_loop()` 检测到标志，进入等待状态

3. **启动电机**（串口命令触发）：
   - 上位机发送 `start` 命令
   - `cli_process()` 解析命令，调用 `pusher_motor_start()`
   - `pusher_motor_start()` 设置 `pusher_motor_start_flag = 1`
   - 主循环中 `pusher_motor_loop()` 检测到标志，进入等待状态

4. **参数配置**：
   - 上位机发送 `set` 命令修改参数
   - `cli_process()` 解析命令，通过 `pusher_motor` 调用 `params_manager` 更新参数
   - 参数自动保存到Flash

### 6. 数据流向

#### 6.1 系统初始化数据流向

1. **Flash → 参数管理器**：系统启动时，`params_manager_init()` 从Flash读取参数
2. **参数管理器 → 电机控制**：`pusher_motor_init()` 根据参数配置硬件（PWM、GPIO等）

#### 6.2 运行时数据流向

1. **外部输入 → 系统**：
   - 外部中断信号 (PB6)：触发电机启动
   - 串口命令：控制电机和修改参数

2. **系统内部数据流向**：
   - 状态机状态转换：`pusher_motor_loop()` 处理状态转换
   - 速度计算：`pusher_motor_calculate_duty_from_speed()` 和 `pusher_motor_calculate_speed_from_duty()`
   - 参数更新：串口命令修改参数后，通过 `params_manager` 更新内存变量

3. **系统 → 外部输出**：
   - PWM信号：控制电机速度
   - GPIO信号：控制电机方向
   - 串口响应：返回命令执行结果

4. **参数管理器 → Flash**：参数修改后，`params_manager_save()` 调用 `FlashStorage_Write()` 永久存储

### 7. 安全与可靠性

- **参数范围检查**：`params_manager` 确保所有参数在有效范围内
- **Flash存储保护**：使用校验和确保数据完整性
- **错误处理**：提供友好的错误提示
- **状态机保护**：确保状态转换的正确性，避免异常状态

### 8. 扩展性

- **模块化设计**：各模块职责清晰，便于扩展
- **可配置参数**：关键参数可通过串口调整，无需重新编译
- **可移植性**：核心逻辑与硬件分离，便于移植到其他平台
- **可扩展性**：可轻松添加新功能和命令

### 9. 代码结构

```
Pusher_Motor/
├── Core/
│   ├── Inc/
│   │   ├── pusher_motor.h     # 电机控制模块头文件
│   │   ├── cli.h              # 串口调试模块头文件
│   │   ├── params_manager.h   # 参数管理模块头文件
│   │   └── flash_storage.h    # Flash存储模块头文件
│   └── Src/
│       ├── pusher_motor.c     # 电机控制模块实现
│       ├── cli.c              # 串口调试模块实现
│       ├── params_manager.c   # 参数管理模块实现
│       ├── flash_storage.c    # Flash存储模块实现
│       ├── stm32f1xx_it.c     # 中断处理函数
│       └── main.c             # 主函数
└── .nn/                      # 项目文档
    ├── HANDOFF.md            # 项目当前状态快照
    ├── plan.md               # 当前工作计划
    └── Memory/
        ├── C-architecture/    # 架构文档
        │   ├── overview.md   # 架构总览
        │   ├── decisions/    # 技术决策
        │   └── contracts/    # 接口契约
        └── E-tasks/
            └── tasks.md       # 总任务表
```

### 10. 技术要点

- **状态机设计**：使用状态机管理电机运行状态，逻辑清晰
- **轮询模式**：串口通信采用轮询模式，避免中断占用
- **参数管理器**：统一管理参数，提供有效性检查和持久化接口
- **Flash存储**：使用Flash存储参数，确保断电后参数不丢失
- **速度计算**：实现了速度与PWM占空比的相互转换
- **命令行接口**：提供了丰富的串口命令，方便调试和配置
- **环形缓冲区**：串口收发使用环形缓冲区，提高数据处理效率

### 11. 系统特性

- **实时性**：状态机处理时间短，响应迅速
- **可靠性**：参数范围检查和Flash数据保护
- **可维护性**：模块化设计，代码结构清晰
- **可扩展性**：易于添加新功能和命令
- **用户友好**：提供了详细的命令帮助和错误提示
