# Pusher Motor 推料电机系统 — 接口文档

本文档描述 Pusher Motor 系统的所有外部接口，包括**串口调试命令**和**C 语言函数 API**。

---

## 1. 串口调试接口（CLI）

### 1.1 串口配置

| 参数 | 值 |
|------|-----|
| 波特率 | 115200 |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验位 | 无 |
| 流控 | 无 |
| 换行符 | `\n`（回车或换行均可触发命令执行） |

### 1.2 命令格式

```
<命令> [子命令] [参数]\n
```

- 命令与子命令之间用**空格**分隔
- 每条命令以回车或换行结尾
- 命令执行后会返回响应文本，并以 `\r\n> ` 提示符结束

### 1.3 命令列表

#### start — 启动推料电机

触发一次推料动作（状态机从 IDLE → WAIT → RUNNING → STOP）。

```
start
```

**响应**：
```
Pusher motor started.
>
```

---

#### set direction_time — 设置运行时间

设置电机单次运行的持续时间。

```
set direction_time <ms>
```

| 参数 | 范围 | 单位 |
|------|------|------|
| `ms` | 1 ~ 60000 | 毫秒 |

**示例**：
```
set direction_time 2500
```

**响应（成功）**：
```
Direction time set to 2500 ms (saved to Flash)
>
```

**响应（失败）**：
```
Error: Invalid value. Range: 1-60000 ms
>
```

---

#### set pwm_duty — 设置 PWM 占空比

直接设置 PWM 占空比数值，控制电机转速。

> **注意**：本项目 PWM 为低电平有效，占空比越小转速越快；500 为停止。

```
set pwm_duty <value>
```

| 参数 | 范围 | 说明 |
|------|------|------|
| `value` | 0 ~ 500 | 0 = 全速，500 = 停止 |

**示例**：
```
set pwm_duty 250
```

**响应（成功）**：
```
PWM duty set to 250 (saved to Flash)
>
```

**响应（失败）**：
```
Error: Invalid value. Range: 0-500
>
```

---

#### set wait_time — 设置等待时间

设置从收到启动信号到电机开始运转之间的延迟时间。

```
set wait_time <ms>
```

| 参数 | 范围 | 单位 |
|------|------|------|
| `ms` | 0 ~ 10000 | 毫秒 |

**示例**：

```
set wait_time 500
```

**响应（成功）**：
```
Wait time set to 500 ms (saved to Flash)
>
```

**响应（失败）**：

```
Error: Invalid value. Range: 0-10000 ms
>
```

---

#### set max_speed — 设置最高转速

设置电机的最高转速（RPM），用于速度计算模型。

```
set max_speed <rpm>
```

| 参数 | 范围 | 单位 |
|------|------|------|
| `rpm` | 1 ~ 10000 | 转/分钟 |

**示例**：

```
set max_speed 3700
```

**响应（成功）**：

```
Max speed set to 3700 RPM (saved to Flash)
>
```

**响应（失败）**：
```
Error: Invalid value. Range: 1-10000 RPM
>
```

---

#### set speed — 设置运行速度

以 cm/分钟 为单位设置运行速度，系统自动计算对应的 PWM 占空比。

```
set speed <cm/min>
```

| 参数 | 范围 | 单位 |
|------|------|------|
| `cm/min` | 0 ~ max_speed × 18.85 | 厘米/分钟（非负） |

**示例**：
```
set speed 60000
```

**响应（成功）**：
```
Speed set to 60000 cm/min (PWM duty: 250) (Range: 0-69708 cm/min) (saved to Flash)
>
```

**响应（失败）**：
```
Error: Invalid value. Must be non-negative (Range: 0-69708 cm/min)
>
```

---

#### set motor_mp_a_dir — 设置 A 方向引脚电平

设置方向控制引脚 PB8 的输出电平。

```
set motor_mp_a_dir <0|1>
```

| 参数 | 值 | 说明 |
|------|-----|------|
| `0` | 低电平 | GPIO_PIN_RESET |
| `1` | 高电平 | GPIO_PIN_SET |

**示例**：
```
set motor_mp_a_dir 0
```

**响应（成功）**：
```
Motor MP A direction set to 0 (saved to Flash)
>
```

**响应（失败）**：
```
Error: Invalid value. Must be 0 or 1
>
```

---

#### set motor_mp_b_dir — 设置 B 方向引脚电平

设置方向控制引脚 PB7 的输出电平。

```
set motor_mp_b_dir <0|1>
```

| 参数 | 值 | 说明 |
|------|-----|------|
| `0` | 低电平 | GPIO_PIN_RESET |
| `1` | 高电平 | GPIO_PIN_SET |

**示例**：
```
set motor_mp_b_dir 1
```

**响应（成功）**：
```
Motor MP B direction set to 1 (saved to Flash)
>
```

---

#### get direction_time — 获取运行时间

```
get direction_time
```

**响应**：
```
Current direction time: 250 ms
>
```

---

#### get pwm_duty — 获取 PWM 占空比

```
get pwm_duty
```

**响应**：
```
Current PWM duty: 150
>
```

---

#### get wait_time — 获取等待时间

```
get wait_time
```

**响应**：
```
Current wait time: 250 ms
>
```

---

#### get max_speed — 获取最高转速

```
get max_speed
```

**响应**：
```
Current max speed: 3655 RPM
>
```

---

#### get speed — 获取当前速度

返回根据当前 PWM 占空比和最高转速计算出的线速度（cm/分钟）。

```
get speed
```

**响应**：
```
Current speed: 60000 cm/min
>
```

---

#### get motor_mp_a_dir — 获取 A 方向引脚状态

```
get motor_mp_a_dir
```

**响应**：
```
Current motor MP A direction: 1
>
```

---

#### get motor_mp_b_dir — 获取 B 方向引脚状态

```
get motor_mp_b_dir
```

**响应**：
```
Current motor MP B direction: 0
>
```

---

#### help / ? — 显示帮助信息

```
help
```

或

```
?
```

**响应**：
```

Available commands:
  start                    - Start pusher motor
  set direction_time <ms>  - Set direction time (1-60000 ms)
  set pwm_duty <value>     - Set PWM duty cycle (0-500)
  set wait_time <ms>       - Set wait time (0-10000 ms)
  set max_speed <rpm>      - Set max speed (1-10000 RPM)
  set speed <cm/min>       - Set speed (0+ cm/min)
  set motor_mp_a_dir <0|1> - Set motor MP A direction
  set motor_mp_b_dir <0|1> - Set motor MP B direction
  set new_pwm_duty <value> - Directly set PWM duty cycle (0-500, not saved)
  get direction_time       - Get current direction time
  get pwm_duty             - Get current PWM duty cycle
  get wait_time            - Get current wait time
  get max_speed            - Get current max speed
  get speed                - Get current speed
  get motor_mp_a_dir       - Get current motor MP A direction
  get motor_mp_b_dir       - Get current motor MP B direction
  get start_signal         - Get MOTOR_PM_ENABLE pin level (PB6)
  help / ?                 - Show this help

>
```

---

## 2. 函数 API 接口

### 2.1 推料电机模块 (`pusher_motor.h`)

#### `pusher_motor_init`

```c
void pusher_motor_init(void);
```

**功能**：初始化推料电机模块。 
**行为**：

- 启动 TIM4_CH4 PWM 输出，初始占空比设为最大（电机停止）
- 从 Flash 读取参数，若有效则使用，否则使用默认值并写入 Flash
- 根据读取的参数设置方向引脚 PB7/PB8 电平
- 状态机复位到 IDLE 
**调用时机**：系统初始化阶段，`main()` 中调用一次。

---

#### `pusher_motor_start`

```c
uint32_t pusher_motor_start(void);
```

**功能**：触发一次推料动作。 
**行为**：仅在系统处于 `IDLE` 模式时设置启动请求并切换到 `RUNNING` 模式，状态机将在下一次 `pusher_motor_loop()` 中从 IDLE 进入 WAIT 状态。 
**调用场景**：

- EXTI 中断回调（外部硬件触发）
- CLI 命令 `start`

---

#### `pusher_motor_loop`

```c
void pusher_motor_loop(void);
```

**功能**：执行电机状态机。 
**行为**：根据当前状态和计时器推进状态转换（IDLE → WAIT → RUNNING → STOP → IDLE）。 
**调用时机**：主循环中周期性调用，必须保证被均匀、频繁地调用。

---

#### `pusher_motor_save_params`

```c
uint32_t pusher_motor_save_params(void);
```

**功能**：将当前所有参数保存到 Flash。 
**返回值**：

- `HAL_OK` (0)：保存成功
- 非零：保存失败 
**调用场景**：CLI `set` 命令执行后自动调用。

---

#### `pusher_motor_calculate_duty_from_speed`

```c
uint32_t pusher_motor_calculate_duty_from_speed(uint32_t speed_cm_min);
```

**功能**：根据线速度（cm/分钟）计算 PWM 占空比。 
**参数**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `speed_cm_min` | `uint32_t` | 目标线速度，单位 cm/min |

**返回值**：PWM 占空比（0 ~ 500） 
**计算公式**：

```
RPM = speed_cm_min / (π × 6)
duty = (MAX_RPM - RPM) / MAX_RPM × 500
```

---

#### `pusher_motor_set_max_speed`

```c
uint32_t pusher_motor_set_max_speed(uint32_t max_speed_rpm);
```

**功能**：设置最高转速并保存到 Flash。 
**参数**：

| 参数 | 类型 | 范围 | 说明 |
|------|------|------|------|
| `max_speed_rpm` | `uint32_t` | 1 ~ 10000 | 最高转速（RPM） |

**返回值**：
- `0`：成功
- `1`：参数无效

---

#### `pusher_motor_set_speed`

```c
uint32_t pusher_motor_set_speed(uint32_t speed_cm_min);
```

**功能**：根据线速度设置 PWM 占空比并保存到 Flash。 
**参数**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `speed_cm_min` | `uint32_t` | 目标线速度，单位 cm/min（非负） |

**返回值**：
- `0`：成功
- 非零：失败 
**行为**：内部调用 `pusher_motor_calculate_duty_from_speed()` 计算占空比，更新 `MOTOR_PWM_DUTY`，并保存到 Flash。

---

#### `pusher_motor_calculate_speed_from_duty`

```c
uint32_t pusher_motor_calculate_speed_from_duty(void);
```

**功能**：根据当前 PWM 占空比和最高转速计算线速度。 
**返回值**：当前线速度，单位 cm/min 
**计算公式**：

```
RPM = (1 - MOTOR_PWM_DUTY / 500) × MOTOR_MAX_SPEED_RPM
speed_cm_min = RPM × (π × 6)
```

---

### 2.2 CLI 模块 (`cli.h`)

#### `cli_init`

```c
void cli_init(UART_HandleTypeDef *huart);
```

**功能**：初始化 CLI 模块。 
**参数**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `huart` | `UART_HandleTypeDef *` | 串口句柄，通常为 `&huart1` |

**行为**：
- 清空接收/发送缓冲区
- 清空命令行缓冲区
- 通过串口打印欢迎信息和提示符 `> ` 
**调用时机**：系统初始化阶段调用一次。

---

#### `cli_process`

```c
void cli_process(void);
```

**功能**：处理 CLI 的输入和输出。 
**行为**：

1. 轮询 USART 接收寄存器，将数据读入接收环形缓冲区
2. 处理发送缓冲区中的回显数据（非阻塞发送）
3. 从接收缓冲区读取数据，进行行编辑处理（回显、退格）
4. 检测到换行后，解析命令并执行
5. 输出响应和提示符 
**调用时机**：主循环中周期性调用。

---

#### `cli_send_string`

```c
void cli_send_string(const char *str);
```

**功能**：以**阻塞方式**发送字符串到串口。 
**参数**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `str` | `const char *` | 要发送的字符串 |

**注意**：此函数使用 `HAL_UART_Transmit(..., HAL_MAX_DELAY)` 阻塞发送，仅用于命令响应输出，不适合高频调用。

---

### 2.3 Flash 存储模块 (`flash_storage.h`)

#### `FlashStorage_Read`

```c
uint32_t FlashStorage_Read(FlashStorage_t *data);
```

**功能**：从 Flash 读取参数。 
**参数**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `data` | `FlashStorage_t *` | 输出参数结构体指针 |

**返回值**：`HAL_OK`（固定返回，实际数据有效性需配合 `FlashStorage_IsValid` 判断）

---

#### `FlashStorage_Write`

```c
uint32_t FlashStorage_Write(FlashStorage_t *data);
```

**功能**：将参数写入 Flash。 
**参数**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `data` | `FlashStorage_t *` | 输入参数结构体指针 |

**行为**：自动计算校验和，先擦除 Flash 页，再逐字写入。 
**返回值**：

- `HAL_OK` (0)：写入成功
- 非零：写入或擦除失败

---

#### `FlashStorage_IsValid`

```c
uint32_t FlashStorage_IsValid(FlashStorage_t *data);
```

**功能**：验证参数数据的有效性。 
**参数**：

| 参数 | 类型 | 说明 |
|------|------|------|
| `data` | `FlashStorage_t *` | 参数结构体指针 |

**返回值**：
- `1`：数据有效（所有字段在合法范围内且校验和正确）
- `0`：数据无效

---

#### `FlashStorage_Erase`

```c
uint32_t FlashStorage_Erase(void);
```

**功能**：擦除 Flash 参数存储页。 
**返回值**：

- `HAL_OK` (0)：擦除成功
- 非零：擦除失败

---

### 2.4 参数管理接口 (`params_manager.h`)

系统通过**参数管理器**统一管理电机运行参数，各模块通过 `params_manager` 提供的 API 读写参数，不再使用全局变量直接访问。

| 参数 | 类型 | 范围 | 默认值 | 说明 |
|------|------|------|--------|------|
| `direction_time_ms` | `uint32_t` | 1 ~ 60000 | 250 | 单次运行时间（ms） |
| `pwm_duty` | `uint32_t` | 0 ~ 500 | 150 | PWM 占空比（0=全速，500=停止） |
| `wait_time_ms` | `uint32_t` | 0 ~ 10000 | 250 | 启动前等待时间（ms） |
| `max_speed_rpm` | `uint32_t` | 1 ~ 10000 | 3655 | 最高转速（RPM） |
| `motor_mp_a_dir` | `uint32_t` | 0 / 1 | 1 | A 方向引脚 PB8 电平 |
| `motor_mp_b_dir` | `uint32_t` | 0 / 1 | 0 | B 方向引脚 PB7 电平 |

---

## 3. 数据类型定义

### 3.1 CLI 命令枚举 (`cli.h`)

```c
typedef enum {
    CLI_CMD_START,              // 启动推料电机
    CLI_CMD_SET_DIRECTION_TIME, // 设置运行时间
    CLI_CMD_SET_PWM_DUTY,       // 设置 PWM 占空比
    CLI_CMD_SET_WAIT_TIME,      // 设置等待时间
    CLI_CMD_SET_MAX_SPEED,      // 设置最高转速
    CLI_CMD_SET_SPEED,          // 设置速度（cm/分钟）
    CLI_CMD_SET_MOTOR_MP_A_DIR, // 设置 A 方向引脚
    CLI_CMD_SET_MOTOR_MP_B_DIR, // 设置 B 方向引脚
    CLI_CMD_SET_NEW_PWM_DUTY,   // 直接设置 PWM 占空比（不保存 Flash）
    CLI_CMD_GET_DIRECTION_TIME, // 获取运行时间
    CLI_CMD_GET_PWM_DUTY,       // 获取 PWM 占空比
    CLI_CMD_GET_WAIT_TIME,      // 获取等待时间
    CLI_CMD_GET_MAX_SPEED,      // 获取最高转速
    CLI_CMD_GET_SPEED,          // 获取速度
    CLI_CMD_GET_MOTOR_MP_A_DIR, // 获取 A 方向引脚
    CLI_CMD_GET_MOTOR_MP_B_DIR, // 获取 B 方向引脚
    CLI_CMD_GET_START_SIGNAL,   // 获取启动信号电平（PB6）
    CLI_CMD_HELP,               // 帮助
    CLI_CMD_UNKNOWN             // 未知命令
} CliCommand_t;
```

### 3.2 Flash 存储结构体 (`flash_storage.h`)

```c
typedef struct {
    uint32_t direction_time_ms;  // 方向运行时间
    uint32_t pwm_duty;           // PWM 占空比
    uint32_t wait_time_ms;       // 等待时间
    uint32_t max_speed_rpm;      // 最高转速（RPM）
    uint32_t motor_mp_a_dir;     // 电机 A 方向引脚状态
    uint32_t motor_mp_b_dir;     // 电机 B 方向引脚状态
    uint32_t checksum;           // 校验和
} FlashStorage_t;
```

### 3.3 CLI 配置结构体 (`cli.h`)

```c
typedef struct {
    UART_HandleTypeDef *huart;  // 串口句柄
    uint8_t rx_buffer[256];     // 接收缓冲区
    uint16_t rx_index;          // 接收缓冲区索引
    CliState_t state;           // CLI 状态
} CliConfig_t;
```

---

## 4. GPIO 引脚定义 (`main.h`)

| 宏定义 | 引脚 | 功能 |
|--------|------|------|
| `MOTOR_PM_ENABLE_Pin` | PB6 | 外部触发输入（EXTI 上升沿） |
| `MOTOR_PM_B_DIRECTION_Pin` | PB7 | 方向控制 B |
| `MOTOR_PM_A_DIRECTION_Pin` | PB8 | 方向控制 A |
| `MOTOR_PM_TIM4_CH4_Pin` | PB9 | PWM 输出（TIM4_CH4） |
