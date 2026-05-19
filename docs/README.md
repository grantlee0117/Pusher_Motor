# Docs

本目录保存仍可直接用于当前开发的项目文档。代码目录自己的说明优先写在对应目录的 `README.md`，仓库结构和协作边界以根目录 `README.md` 为准。

## 文档索引

| 文件 | 说明 |
|------|------|
| `Build.md` | 固件编译构建说明 |
| `串口调试命令.md` | 串口 CLI 调试命令记录 |
| `deprecated/` | 已过时但暂时保留的历史文档 |

## 相关入口

| 入口 | 说明 |
|------|------|
| `../README.md` | 仓库总入口、目录结构和协作边界 |
| `Build.md` | Arm GNU Toolchain + CMake + Ninja 编译流程 |
| `../App/README.md` | 手写固件业务代码分层说明 |
| `../Core/README.md` | CubeMX 生成代码边界说明 |
| `../Config/README.md` | 构建和硬件相关配置说明 |
| `../hardware/README.md` | 硬件资料目录和新板资料说明 |
| `../upper_computer/README.md` | 上位机运行和打包说明 |

## 维护约定

- 修改串口命令、上位机协议或固件行为时，同步更新 `串口调试命令.md`。
- 修改硬件接口或新板资料时，同步检查 `hardware/` 下的 README。
- 文档尽量写清楚事实来源，例如代码文件、IO 表、原理图或测试记录。
- 如果文档与当前代码严重不匹配，但仍有历史参考价值，移动到 `deprecated/` 并在 `deprecated/README.md` 中说明原因。
