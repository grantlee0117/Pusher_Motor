# Pusher Motor 售后调试上位机（便携版）

## 运行方式

需要 Python 3.9+ 和 `pyserial`：

```bat
py -m pip install -r requirements.txt
py app.py
```

## 打包成 exe（无需对方装 Python）

### 单文件模式（推荐）
双击执行：

```bat
build_windows.bat
```

产物在 `dist\PusherMotorHost.exe`，直接发给对方即可。

### 文件夹模式
双击执行：

```bat
build_windows_folder.bat
```

产物在 `dist\PusherMotorHost\`，**必须发送整个文件夹**。

## 文件说明

| 文件 | 说明 |
|------|------|
| `app.py` | 主程序（GUI） |
| `pusher_motor_serial.py` | 串口通信模块 |
| `requirements.txt` | 依赖列表 |
| `build_windows.bat` / `build_windows_folder.bat` | 打包脚本 |
| `pusher_motor_host.spec` / `pusher_motor_host_folder.spec` | PyInstaller 配置 |

## 串口参数

默认 `115200, 8N1`。
