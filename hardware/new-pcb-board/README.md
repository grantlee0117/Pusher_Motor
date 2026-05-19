# New PCB Board

本目录保存后续新板资料。资料显示该板为 `Motor_Push_Board V1.0 260427`，主控为 `STM32F103C8T6`，外部晶振 `8MHz`。

## 概述

- 用途：后续硬件替换和固件适配的参考资料。
- 适配边界：在修改代码前，需要先对照 IO 分配表确认 `Pusher_Motor.ioc`、GPIO、TIM、USART、ADC/EXTI 等配置。

## 文件索引

```text
new-pcb-board/
├── schematic/
│   ├── README.md
│   └── Motor_Push_Board_V1.0_260427.sch
├── interface/
│   ├── README.md
│   ├── pusher-motor-board-io-map.xlsx
│   ├── io-map-page-01.png
│   ├── io-map-page-02.png
│   └── io-map-page-03.png
└── board-view/
    ├── README.md
    └── connector-overview.png
```

## 固件适配前检查清单

- MCU 型号、封装和时钟来源是否与 `Pusher_Motor.ioc` 一致。
- USART 调试口是否仍使用 USART1，TX/RX 引脚是否一致。
- 电机接口的 PWM、方向、刹车、反馈、使能等信号是否和当前代码配置一致。
- 输入信号、到位传感器、SWD、24V 供电接口是否和文档说明一致。
- 根目录 README 和 `docs/串口调试命令.md` 是否需要更新。
