# Build Guide


本文档说明如何在 Linux 环境下使用 Arm GNU Toolchain (`arm-none-eabi-gcc`) + CMake + Ninja 编译固件。

---

## 依赖环境

需要确保你已经安装并能在命令行访问以下工具：

| 工具 | 用途 |
|------|------|
| `arm-none-eabi-gcc` | STM32F103 固件交叉编译器 |
| `arm-none-eabi-g++` | 链接器由工具链文件引用 |
| `arm-none-eabi-objcopy` | 构建后生成 `.hex` 和 `.bin` |
| `cmake` | 配置和驱动构建流程 |
| `ninja` | CMake preset 使用的构建后端 |

版本要求：

- CMake 3.22+
- Ninja 可用即可
- Arm GNU Toolchain 需要支持 Cortex-M3 和 `arm-none-eabi-*` 命令前缀

检查命令：

```bash
arm-none-eabi-gcc --version
arm-none-eabi-g++ --version
arm-none-eabi-objcopy --version
cmake --version
ninja --version
```

如果提示 `command not found`，请先安装对应工具。

---

## 构建方式概览

当前仓库的固件构建入口是 CMake preset：

- `CMakePresets.json` 定义 `Debug` 和 `Release` 两个 preset。预先配置好了两种构建模式（Debug 调试版，Release 发布版），让你不用手动输入复杂参数。
- `cmake/gcc-arm-none-eabi.cmake` 定义交叉编译工具链。告诉编译器使用“ARM 嵌入式芯片”的专用工具链（gcc-arm-none-eabi）
- `CMakeLists.txt` 定义固件目标、源文件、include 路径和 `.hex`、`.bin` 产物生成。它是具体规则，比如编译哪些 .c 文件、去哪里找头文件、最终生成 .hex 或 .bin 固件文件。
- 根目录 `Makefile` 和 `Scripts/build.sh` 是便捷封装，不是另一套独立构建系统。

日常开发优先使用 CMake preset；熟悉后也可以用 `make` 或脚本简化命令。

---

## Debug 还是 Release？

| | Release | Debug |
|--|----------------------|-------|
| **用途** | 生成最终烧录固件 | 单步调试代码 |
| **体积** | 小 | 大（含调试信息） |
| **优化** | 尺寸优化 | 低优化，便于断点 |
| **产物位置** | `build/Release/` | `build/Debug/` |

Release 体积更小、做尺寸优化、产物在 build/Release/，适合作为最终烧录固件；Debug 则保留调试信息、体积更大、低优化，适合单步调试。

- 如果你只是想生成一个可烧录的 .hex 文件，建议先用 Release。
- 如果你要接调试器、打断点、单步跟代码，建议用 Debug。


在嵌入式项目中，Release 可能因为优化导致变量被优化掉、代码执行顺序和源码不完全对应，调试体验会差；Debug 更适合定位问题。 但反过来，Debug 固件体积更大，有时也可能因为时序、内存占用、优化差异，表现和 Release 不完全一致。

**如果你是第一次编译，并且你什么都不懂，直接选 Release**，你会得到可以直接烧录的 `.hex` 文件。

---

## 编译 Release 固件

先 cd 进入项目根目录（包含 `CMakeLists.txt` 和 `CMakePresets.json` 的目录）。然后执行:

```bash
ls -la | grep -E "CMakeLists.txt|CMakePresets.json"
# 确认一下当前目录有什么
```
应该能看到这两个文件，即 `CMakeLists.txt` 和 `CMakePresets.json`


### 1. 配置

```bash
cmake --preset Release
```

**这个命令做什么**：让 CMake 读取 `CMakePresets.json` 中的 `Release` 配置，自动创建 `build/Release/` 文件夹，并生成 Ninja 能用的构建文件。


**成功标志**：最后一行输出 `Build files have been written to: .../build/Release`


**如果报错**：
- `Unknown argument: --preset` → CMake 版本太低（需要 3.22+），用 `cmake --version` 检查
- `Could not find CMAKE_ROOT` → CMake 安装有问题
- `arm-none-eabi-gcc: command not found` → 工具链没装或 PATH 没配



### 2. 编译（Build）

```bash
cmake --build --preset Release
```

这一步实际调用编译器（arm-none-eabi-gcc）编译所有 .c 文件，生成固件文件。

**成功标志**：最后一行输出 `[100%] Built target Pusher_Motor`


**如果报错**：
- `ninja: command not found` → 没装 Ninja，或者 CMake 没用 Ninja 后端（检查 build/Debug/build.ninja 是否存在）
- `No such file or directory` 某个 .h 文件 → 源文件路径配置有问题
- 编译错误（具体的 C 语法错误）→ 修代码



### 3. 找到产物

```bash
ls build/Release/Pusher_Motor.*
```

你应该看到：

| 文件 | 本质 | 说明 |
|---|---|---|
| `Pusher_Motor.hex` | Intel HEX 固件 | 烧录用这个 |
| `Pusher_Motor.bin` | 原始二进制固件 | 烧录也可以用这个 |
| `Pusher_Motor.elf` | ELF 调试/链接产物 | 调试用（最大，包含调试信息） |
| `Pusher_Motor.map` | 链接映射文件 | 内存映射信息 |

### 4. 一键命令（可选）

```bash
cmake --preset Release && cmake --build --preset Release
# `&&` 表示前一条成功后才执行后一条。
```
这条命令就是先配置构建系统，如果配置成功，就编译生成固件。两步一条命令完成，既高效又安全。


---

## 编译 Debug 固件（调试用）

如果你需要单步调试代码，用 Debug 构建。
cd 进入项目根目录（包含 `CMakeLists.txt` 和 `CMakePresets.json` 的目录），然后执行:

```bash
ls -la | grep -E "CMakeLists.txt|CMakePresets.json"
# 确认一下当前目录有什么
```
应该能看到这两个文件，即 `CMakeLists.txt` 和 `CMakePresets.json`


### 1. 配置

```bash
cmake --preset Debug
```

**这个命令做什么**：让 CMake 读取 `CMakePresets.json` 中的 `Debug` 配置，自动创建 `build/Debug/` 文件夹，并生成 Ninja 能用的构建文件。


**成功标志**：最后一行输出 `Build files have been written to: .../build/Debug`


**如果报错**：
- `Unknown argument: --preset` → CMake 版本太低（需要 3.22+），用 `cmake --version` 检查
- `Could not find CMAKE_ROOT` → CMake 安装有问题
- `arm-none-eabi-gcc: command not found` → 工具链没装或 PATH 没配

### 2. 编译

```bash
cmake --build --preset Debug
```

这一步实际调用编译器（arm-none-eabi-gcc）编译所有 .c 文件，生成固件文件。


**成功标志**：最后一行输出 `[100%] Built target Pusher_Motor`

**如果报错**：
- `ninja: command not found` → 没装 Ninja，或者 CMake 没用 Ninja 后端（检查 build/Debug/build.ninja 是否存在）
- `No such file or directory` 某个 .h 文件 → 源文件路径配置有问题
- 编译错误（具体的 C 语法错误）→ 修代码

### 3. 找到产物

```bash
ls build/Debug/Pusher_Motor.*
```

你应该看到：

| 文件 | 本质 | 说明 |
|---|---|---|
| `Pusher_Motor.hex` | Intel HEX 固件 | 烧录用这个 |
| `Pusher_Motor.bin` | 原始二进制固件 | 烧录也可以用这个 |
| `Pusher_Motor.elf` | ELF 调试/链接产物 | 调试用（Debug 版本包含调试符号信息） |
| `Pusher_Motor.map` | 链接映射文件 | 内存映射信息 |

> **Note**：Debug 固件体积通常比 Release 大，因为包含了调试符号且未做优化。这很正常，不影响调试使用。

### 4. 一键命令（可选）

```bash
cmake --preset Debug && cmake --build --preset Debug
# `&&` 表示前一条成功后才执行后一条。
```
这条命令就是先配置构建系统，如果配置成功，就编译生成固件。两步一条命令完成，既高效又安全。


---

## 快速对比

| 构建类型 | 配置命令 | 编译命令 | 产物目录 |
|---------|---------|---------|---------|
| Release | `cmake --preset Release` | `cmake --build --preset Release` | `build/Release/` |
| Debug | `cmake --preset Debug` | `cmake --build --preset Debug` | `build/Debug/` |

> **提示**：Release 固件体积小、速度快，适合最终烧录；Debug 固件包含调试符号，适合在调试器中单步跟踪代码。


---

## 其他常用工作流命令

前面已经讲了最基本的两步，配置和编译：

```bash
cmake --preset Release
cmake --build --preset Release
```

这两步背后的本质是：

1. **配置**：CMake 读取 `CMakePresets.json`、`CMakeLists.txt` 和工具链文件，然后生成一个构建目录，比如 `build/Release/`。
2. **编译**：CMake 再调用 Ninja，使用 `build/Release/` 里面已经生成好的构建文件去编译源码。

所以很多命令看起来像是在“重新编译”，但实际上它们处理的是不同层次的问题：有的只是重新编译 `.c` 文件，有的是重新生成构建目录里的配置文件，有的是清理编译产物。

### 仅重新编译

如果只是修改了 `.c` 或 `.h` 文件，通常不需要重新配置，直接 build 即可。

```bash
cmake --build --preset Release
```

或者 Debug：

```bash
cmake --build --preset Debug
```

这会复用已有的 `build/Release/` 或 `build/Debug/` 配置目录，只重新编译发生变化的文件。

### 重新配置

如果修改了 `CMakeLists.txt`、`CMakePresets.json`、工具链文件、链接脚本，或者你怀疑构建目录里的配置已经不可靠，可以重新配置。

```bash
cmake --preset Release
```

这会重新检查配置并更新 `build/Release/` 中的构建文件，但不会主动删除所有旧的编译产物。

### 使用 fresh 重新配置

`--fresh` 可以理解为“重新生成 CMake 配置缓存”。

```bash
cmake --fresh --preset Release
```

它比普通 `cmake --preset Release` 更干净，适合在这些情况下使用：

- 切换工具链或修改工具链文件后。
- 修改 `CMakePresets.json` 后。
- 修改链接脚本路径或重要编译选项后。
- CMake 配置结果看起来不对，但又不想手动删除整个 `build/` 目录时。

Debug 同理：

```bash
cmake --fresh --preset Debug
```

### 清理编译产物

如果只是想清掉某个 preset 已经编译出来的目标文件和固件产物，可以执行：

```bash
cmake --build --preset Release --target clean
```

或者：

```bash
cmake --build --preset Debug --target clean
```

这类 clean 是“清理编译产物”，不是“重新生成配置”。清理后再次执行 build，会重新编译，但仍然使用原来的 CMake 配置目录。

### 彻底重来

如果构建目录已经混乱，或者你想完全从零开始，可以删除对应的构建目录，然后重新配置和编译。

```bash
rm -rf build/Release
cmake --preset Release
cmake --build --preset Release
```

Debug 同理：

```bash
rm -rf build/Debug
cmake --preset Debug
cmake --build --preset Debug
```

这比 `clean` 更彻底，因为它连 CMake 已经生成的配置目录也一起删掉了。日常开发一般不需要这么做，只有在构建目录状态异常时再用。

---

## 常用工作流

下面按实际场景列出常用命令。大多数情况下，不需要每次都从头配置；先判断自己改了什么，再选对应流程。

### 1. 项目刚起步，第一次编译

第一次拿到仓库时，先确认依赖环境，再做一次完整配置和编译。

```bash
arm-none-eabi-gcc --version
cmake --version
ninja --version
```

如果只是想先得到一个可以烧录的固件，建议用 Release：

```bash
cmake --preset Release
cmake --build --preset Release
```

如果想一步到位，也可以选择执行：

```bash
cmake --preset Release && cmake --build --preset Release
# `&&` 表示前一条成功后才执行后一条。
```


确认产物：

```bash
ls build/Release/Pusher_Motor.*
```

如果你是要接调试器单步调试，则改用 Debug：

```bash
cmake --preset Debug
cmake --build --preset Debug
```

### 2. 正常开发，只改了业务代码

例如只改了 `App/` 下面的 `.c`、`.h` 文件，通常只需要重新 build。

```bash
cmake --build --preset Release
```

如果当前在调试：

```bash
cmake --build --preset Debug
```

不需要每次都执行 `cmake --preset ...`，因为配置目录已经存在，CMake/Ninja 会自动判断哪些文件需要重新编译。

### 3. 修改了 CMake、工具链或链接脚本

如果修改了这些文件：

- `CMakeLists.txt`
- `CMakePresets.json`
- `cmake/gcc-arm-none-eabi.cmake`
- `Config/ldscripts/STM32F103XX_FLASH.ld`

建议先 fresh 配置，再编译。

```bash
cmake --fresh --preset Release
cmake --build --preset Release
```

Debug：

```bash
cmake --fresh --preset Debug
cmake --build --preset Debug
```

这类修改会影响“怎么编译、怎么链接、产物放哪里”，只 build 有时不够明确，fresh 一次更稳。

### 4. 项目换板子，或者 CubeMX 重新生成代码

如果后续新板适配涉及 `Pusher_Motor.ioc`、`Core/`、外设初始化、引脚定义、定时器通道或链接脚本变化，建议按“重新配置”的思路处理。

推荐流程：

```bash
cmake --fresh --preset Release
cmake --build --preset Release
```

如果编译失败，先看错误属于哪一类：

- 找不到头文件：检查 include 路径或 CubeMX 生成文件是否完整。
- 找不到符号：检查源文件是否加入 `CMakeLists.txt`。
- 链接脚本或内存区域错误：检查 `Config/ldscripts/`。
- 引脚宏不存在：检查 `Core/Inc/main.h` 和 CubeMX 生成结果。

换板子时，编译通过只代表代码能生成固件，不代表硬件行为一定正确。还需要继续核对 `hardware/` 中的新板资料、`Pusher_Motor.ioc` 和实际引脚功能。

### 5. 只想清理后重新编译

如果只是想让当前 preset 重新编译一遍，但不想重新生成 CMake 配置，可以 clean 后 build。

```bash
cmake --build --preset Release --target clean
cmake --build --preset Release
```

Debug：

```bash
cmake --build --preset Debug --target clean
cmake --build --preset Debug
```

这适合怀疑某些目标文件没有正确更新，但 CMake 配置本身没有变化的情况。

### 6. 构建目录异常，想彻底重来

如果出现很奇怪的构建问题，比如配置缓存残留、preset 切换后路径异常、工具链切换后仍然引用旧路径，可以直接删除对应构建目录。

```bash
rm -rf build/Release
cmake --preset Release
cmake --build --preset Release
```

Debug：

```bash
rm -rf build/Debug
cmake --preset Debug
cmake --build --preset Debug
```

这个流程最干净，但也最重。日常开发优先使用 build 或 fresh，只有确认构建目录状态异常时再彻底删除。

### 7. 快速确认 Release 和 Debug 都能编译

在提交前，如果想确认两种模式都没有坏，可以分别构建：

```bash
cmake --fresh --preset Release
cmake --build --preset Release

cmake --fresh --preset Debug
cmake --build --preset Debug
```

如果两个 preset 都能通过，至少说明当前代码、构建配置和工具链文件在两种常用模式下都能工作。

### 8. 想看具体编译命令

如果需要排查某个 `.c` 文件实际用了哪些编译参数，可以打开 verbose 输出。

```bash
cmake --build --preset Release --verbose
```

Debug：

```bash
cmake --build --preset Debug --verbose
```

这会显示更完整的编译和链接命令，适合排查 include 路径、宏定义、优化参数和链接参数。
