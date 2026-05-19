# Scripts 目录

放自动化脚本和临时迁移保留文件。

- `build.sh`: 使用 CMake preset 配置并构建固件
- `legacy-generated/`: 整理前残留在根目录的 CMake 生成文件备份；日常构建使用仓库根目录的新 `Makefile`

## 使用约定

- 日常构建优先使用根目录 `make` 或 `./Scripts/build.sh`。
- 脚本应保持可重复执行，不依赖个人机器上的临时路径。
- 迁移或备份文件如果长期保留，需要在 `legacy-generated/` 或对应 README 中说明来源和用途。
