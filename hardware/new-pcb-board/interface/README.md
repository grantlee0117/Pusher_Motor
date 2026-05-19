# Interface Map

本目录保存新板 IO 分配资料。

## 文件说明

| 文件 | 说明 |
|------|------|
| `pusher-motor-board-io-map.xlsx` | IO 分配表源文件，应作为接口定义的主要事实来源 |
| `io-map-page-01.png` | IO 分配表截图，第 1 段，包含 E1/E2/B1/B2 等电机接口 |
| `io-map-page-02.png` | IO 分配表截图，第 2 段，包含 B/A 类电机接口和供电接口 |
| `io-map-page-03.png` | IO 分配表截图，第 3 段，包含 USART、SWD、启动信号、传感器和 24V 输入 |

## 阅读顺序

先查看 `pusher-motor-board-io-map.xlsx`，再用 PNG 文件做快速浏览或评审截图。若截图和 Excel 内容不一致，以 Excel 文件为准。

## 固件关联点

适配新板时，需要重点核对：

- GPIO 输出：方向、刹车、启动/停止等控制信号。
- TIM 通道：PWM 调速、速度反馈等复用功能。
- USART1：调试串口 RX/TX。
- ADC/EXTI：启动信号、传感器输入等检测信号。
- 电源和地线：24V、3.3V、GND 的接口定义。
