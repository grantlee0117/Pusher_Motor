# Pusher Motor

STM32F103 推料电机控制固件，包含 PWM 电机控制、参数 Flash 持久化和 USART1 串口 CLI。

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
├── docs/                        # 设计、接口、串口命令等文档
├── Scripts/                     # 自动化脚本
├── upper_computer/              # 上位机工具
├── Pusher_Motor.ioc             # CubeMX 配置文件，硬件配置事实来源
├── CMakeLists.txt               # 顶层构建入口
├── CMakePresets.json            # Debug/Release 构建预设
└── Makefile                     # CMake 便捷封装
```

## 代码归属规则

- `Core/` 只放 CubeMX 生成或强绑定 CubeMX 的代码。`main.c` 中只保留初始化顺序和 `App/System` 入口调用。
- 新增业务功能优先放到 `App/`，不要继续塞进 `Core/Src` 或 `Core/Inc`。
- 硬件外设初始化仍由 `MX_GPIO_Init()`、`MX_TIM4_Init()`、`MX_USART1_UART_Init()` 管理。
- `Pusher_Motor.ioc` 保持在仓库根目录，便于 CubeMX/CubeIDE 直接打开；修改硬件配置后需要同步检查生成代码差异。
- 链接脚本放在 `Config/ldscripts/`，工具链文件通过该路径引用。

## 构建

依赖：

- CMake 3.22+
- Ninja
- `arm-none-eabi-gcc` 工具链

常用命令：

```bash
make
make BUILD_PRESET=Release
./Scripts/build.sh Debug
```

等价的 CMake 命令：

```bash
cmake --preset Debug
cmake --build --preset Debug
```

构建产物输出到 `build/Debug/` 或 `build/Release/`，包括 `.elf`、`.hex`、`.bin` 和 `.map`。

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

- [docs/Project_Description.md](docs/Project_Description.md): 固件功能和流程说明
- [docs/Design.md](docs/Design.md): 设计文档
- [docs/Interface.md](docs/Interface.md): 对外接口说明
- [upper_computer/README.md](upper_computer/README.md): 上位机说明
