# Config 目录

集中放不会由 CubeMX 频繁改写、但会影响固件构建或硬件布局的配置。

- `ldscripts/`: GCC/Clang 链接脚本

`Pusher_Motor.ioc` 仍保留在仓库根目录，便于 CubeMX/CubeIDE 直接识别项目。

## 维护约定

- 构建工具链或链接地址变化时，优先检查 `cmake/` 中的工具链文件是否引用了这里的路径。
- 应用层运行参数不放在本目录，应放在 `App/Config/`。
- 新增配置文件时，需要在本 README 中说明它是否影响构建、烧录或硬件布局。
