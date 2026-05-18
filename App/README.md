# App 目录

`App/` 是手写业务代码区。CubeMX 重新生成代码时，不应覆盖这里。

- `System/`: 应用入口，负责组合初始化和主循环调度
- `Control/`: 电机控制、状态机、运动参数计算
- `Communication/`: 串口 CLI 和后续协议处理
- `Storage/`: Flash、EEPROM、外部存储等持久化封装
- `Config/`: 应用参数结构、校验、加载和保存策略

新增功能优先放在这里；只有外设初始化、中断模板、HAL 配置才放回 `Core/`。
