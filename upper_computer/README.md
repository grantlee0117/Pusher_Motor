# Pusher Motor 售后调试上位机

这是给售后工程师使用的 Windows 桌面上位机。界面按钮背后仍然使用固件已有的串口 CLI 命令，不需要售后手动输入命令。

## 运行

1. 在 Windows 安装 Python 3.9 或更新版本。
2. 进入本目录。
3. 安装依赖：

```bat
py -m pip install -r requirements.txt
```

4. 启动：

```bat
py app.py
```

串口参数默认使用 `115200, 8N1`。

## 打包

在 Windows 中双击或执行：

```bat
build_windows.bat
```

默认会打包成单文件 exe，产物在：

```text
dist\PusherMotorHost.exe
```

把这个 `PusherMotorHost.exe` 发给同事即可，对方不需要安装 Python 或依赖库。

如果需要备用的文件夹模式，可以执行：

```bat
build_windows_folder.bat
```

文件夹模式的产物是 `dist\PusherMotorHost\`，发送时必须发送整个文件夹，不能只发送里面的 exe。

如果对方提示“加载 Python DLL 失败”，优先检查是否误发了文件夹模式里的单独 exe。单文件模式请发送 `dist\PusherMotorHost.exe`。

## 支持的功能

- 连接/断开串口。
- 读取全部主要参数。
- 手动做一次单次运行测试。
- 读取启动信号状态。
- 设置运行时间、等待时间。
- 设置速度和加速度，常用参数支持输入框和滑条。
- 设置电机 A/B 方向。
- 读取 PB6 启动信号电平。
- 在状态栏显示当前系统模式。

普通界面不暴露系统模式按钮。需要临时调试速度或 PWM 时，上位机会自动进入 `SERVICE` 模式。

界面里的“加速度”是售后友好的 0-100 数值，数值越大启动越快。程序会自动换算为固件里的 `accel_time`。

## 固件命令对应关系

| 界面功能 | 串口命令 |
| --- | --- |
| 启动电机 | `start` |
| 系统模式显示 | `get mode` |
| 运行时间 | `set direction_time <ms>` / `get direction_time` |
| 等待时间 | `set wait_time <ms>` / `get wait_time` |
| 加速度 | `set accel_time <ms>` / `get accel_time`，界面自动换算 |
| 速度应用 | `set mode service` + `set new_pwm_duty <0-500>` |
| 速度保存 | `set speed <cm/min>` / `get speed` |
| 高级 PWM | `set new_pwm_duty <0-500>` 或 `set pwm_duty <0-500>` |
| 速度校准 | `set max_speed <rpm>` / `get max_speed` |
| 电机 A 方向 | `set motor_mp_a_dir <0|1>` / `get motor_mp_a_dir` |
| 电机 B 方向 | `set motor_mp_b_dir <0|1>` / `get motor_mp_b_dir` |
| 启动信号 | `get start_signal` |

## 常用动作说明

- `单次运行测试`：上位机发送 `start`，让设备按当前参数执行一次输送动作，用来测试设置是否合适。
- `读取启动信号`：读取控制板 PB6 输入脚当前是高电平还是低电平，用来判断外部设备有没有给启动信号。
- `启动信号`：显示上一次读取到的 PB6 电平状态，`HIGH` 表示启动信号当前有效，`LOW` 表示当前无启动信号。
