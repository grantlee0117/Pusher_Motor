# Pusher Motor

STM32F103 推料电机控制固件，包含 PWM 电机控制、参数 Flash 持久化和 USART1 串口 CLI。

本仓库同时保存固件、上位机、项目文档和硬件资料。根目录 README 是团队入口：新成员应先看这里，再根据任务跳到对应目录。

## 快速入口

| 目标 | 入口 |
|------|------|
| 编译固件 | `docs/Build.md` |
| 修改业务逻辑 | `App/`，说明见 `App/README.md` |
| 查看 CubeMX 生成代码边界 | `Core/`，说明见 `Core/README.md` |
| 查看串口命令 | `docs/串口调试命令.md` |
| 查看新板硬件资料 | `hardware/README.md` |
| 运行或打包上位机 | `upper_computer/README.md` |
| 查看历史过期文档 | `docs/deprecated/README.md` |

## 目录结构

```text
.
├── App/                         # 手写业务代码
│   ├── Communication/           # 串口 CLI、协议交互
│   ├── Config/                  # 应用参数管理
│   ├── Control/                 # 推料电机控制逻辑
│   ├── Storage/                 # Flash 存储封装
│   └── System/                  # 应用层初始化和主循环入口
├── Core/                        # STM32CubeMX 生成/维护的代码
│   ├── Inc/                     # main.h、外设头文件、HAL 配置
│   ├── Src/                     # main.c、gpio/tim/usart、中断、系统文件
│   └── Startup/                 # startup_stm32f103xb.s
├── Drivers/                     # CMSIS 和 STM32 HAL 驱动
├── Config/
│   └── ldscripts/               # 链接脚本
├── cmake/                       # CMake 工具链和 CubeMX CMake 入口
├── docs/                        # 当前文档索引、串口命令、构建教程和过期历史文档
├── hardware/                    # 硬件资料，当前包含后续新板资料
├── Scripts/                     # 自动化脚本
├── upper_computer/              # 上位机工具
├── Pusher_Motor.ioc             # CubeMX 配置文件，硬件配置事实来源
├── CMakeLists.txt               # 顶层构建入口
├── CMakePresets.json            # Debug/Release 构建预设
└── Makefile                     # CMake 便捷封装
```

## 目录和协作边界

- `Core/` 只放 CubeMX 生成或强绑定 CubeMX 的代码。`main.c` 中只保留初始化顺序和 `App/System` 入口调用。
- 新增业务功能优先放到 `App/`，不要继续塞进 `Core/Src` 或 `Core/Inc`。
- 硬件外设初始化仍由 `MX_GPIO_Init()`、`MX_TIM4_Init()`、`MX_USART1_UART_Init()` 管理。
- `Pusher_Motor.ioc` 保持在仓库根目录，便于 CubeMX/CubeIDE 直接打开；修改硬件配置后需要同步检查生成代码差异。
- 链接脚本放在 `Config/ldscripts/`，工具链文件通过该路径引用。
- `hardware/` 只放硬件资料，不参与固件构建。当前 `hardware/new-pcb-board/` 是后续新板资料，整理资料不代表固件已经适配新板。
- `docs/` 放当前仍可直接使用的说明文档；接口、串口命令、硬件资料发生变化时，需要同步更新相关文档索引。
- 上位机代码只放在 `upper_computer/`；打包产物、Python 缓存和虚拟环境不要提交。
- 提交前检查 `git status --short`，确认只包含本次有意修改的文件。

## 固件编译

固件使用 Arm GNU Toolchain (`arm-none-eabi-gcc`) + CMake + Ninja 编译。完整环境检查、Debug/Release 构建流程、产物位置和常见问题见 [docs/Build.md](docs/Build.md)。

## 运行流程

启动后 `Core/Src/main.c` 完成 HAL、系统时钟和外设初始化，然后调用：

```c
app_system_init(&huart1);
```

主循环只调度应用层：

```c
while (1)
{
    app_system_loop();
}
```

`app_system_init()` 初始化推料电机和 CLI；`app_system_loop()` 处理串口命令和电机状态机。

## 串口控制

USART1 默认用于 CLI。连接后输入：

```text
help
start
set speed 100
get speed
```

更完整的命令说明见 [docs/串口调试命令.md](docs/串口调试命令.md)。

## 文档

- [docs/README.md](docs/README.md): 当前文档索引
- [docs/Build.md](docs/Build.md): 固件编译构建说明
- [docs/串口调试命令.md](docs/串口调试命令.md): 串口 CLI 调试命令
- [docs/deprecated/README.md](docs/deprecated/README.md): 过期历史文档说明
- [hardware/README.md](hardware/README.md): 硬件资料目录和维护约定
- [upper_computer/README.md](upper_computer/README.md): 上位机说明
