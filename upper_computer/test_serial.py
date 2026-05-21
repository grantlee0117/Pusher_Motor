import serial
import time
import sys

PORT = "COM18"
BAUDRATE = 115200
TIMEOUT = 2.0


def send_cmd(ser, cmd, desc="", delay=0.3):
    """发送命令并读取响应"""
    print(f">>> 发送: {cmd}" + (f" ({desc})" if desc else ""))
    ser.write((cmd + "\r\n").encode())
    ser.flush()

    time.sleep(delay)
    response = ser.read(ser.in_waiting).decode('utf-8', errors='replace')

    if response:
        print(f"<<< 响应:\n{response}")
    else:
        print("<<< ⚠️ 无响应")
    print("-" * 50)
    return response


def main():
    try:
        ser = serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT)
        print(f"✅ 已连接 {PORT} @ {BAUDRATE}bps")
        print("等待设备启动...\n")

        # 先读取启动信息
        time.sleep(0.5)
        boot_data = ser.read(ser.in_waiting).decode('utf-8', errors='replace')
        if boot_data:
            print(f"【启动信息】\n{boot_data}")
            print("=" * 50)

        # ==========================================================
        #  文档中所有命令的完整测试
        # ==========================================================

        print("\n========== 1. 帮助命令 ==========")
        send_cmd(ser, "help", "帮助命令")
        send_cmd(ser, "?", "帮助命令(简写)")

        print("\n========== 2. 启动电机 ==========")
        send_cmd(ser, "start", "启动推料电机")

        print("\n========== 3. 运行时间(direction_time) ==========")
        send_cmd(ser, "get direction_time", "获取当前运行时间")
        send_cmd(ser, "set direction_time 2500", "设置运行时间 2500ms")
        send_cmd(ser, "get direction_time", "验证设置结果")
        send_cmd(ser, "set direction_time 0", "测试越界值 0")
        send_cmd(ser, "set direction_time 60001", "测试越界值 60001")

        print("\n========== 4. PWM占空比(pwm_duty) ==========")
        send_cmd(ser, "get pwm_duty", "获取当前PWM占空比")
        send_cmd(ser, "set pwm_duty 50", "设置PWM占空比 50")
        send_cmd(ser, "get pwm_duty", "验证设置结果")
        send_cmd(ser, "set pwm_duty 101", "测试越界值 101")

        print("\n========== 5. 等待时间(wait_time) ==========")
        send_cmd(ser, "get wait_time", "获取当前等待时间")
        send_cmd(ser, "set wait_time 500", "设置等待时间 500ms")
        send_cmd(ser, "get wait_time", "验证设置结果")
        send_cmd(ser, "set wait_time 10001", "测试越界值 10001")

        print("\n========== 6. 最高转速(max_speed) ==========")
        send_cmd(ser, "get max_speed", "获取当前最高转速")
        send_cmd(ser, "set max_speed 3700", "设置最高转速 3700 RPM")
        send_cmd(ser, "get max_speed", "验证设置结果")
        send_cmd(ser, "set max_speed 0", "测试越界值 0")

        print("\n========== 7. 速度(speed) ==========")
        send_cmd(ser, "get speed", "获取当前速度")
        send_cmd(ser, "set speed 60000", "设置速度 60000 cm/min")
        send_cmd(ser, "get speed", "验证设置结果")
        send_cmd(ser, "set speed -1", "测试越界值 -1")

        print("\n========== 8. 电机A方向(motor_mp_a_dir) ==========")
        send_cmd(ser, "get motor_mp_a_dir", "获取电机A方向")
        send_cmd(ser, "set motor_mp_a_dir 0", "设置电机A方向 0")
        send_cmd(ser, "get motor_mp_a_dir", "验证设置结果")
        send_cmd(ser, "set motor_mp_a_dir 1", "设置电机A方向 1")
        send_cmd(ser, "get motor_mp_a_dir", "验证设置结果")
        send_cmd(ser, "set motor_mp_a_dir 2", "测试越界值 2")

        print("\n========== 9. 电机B方向(motor_mp_b_dir) ==========")
        send_cmd(ser, "get motor_mp_b_dir", "获取电机B方向")
        send_cmd(ser, "set motor_mp_b_dir 1", "设置电机B方向 1")
        send_cmd(ser, "get motor_mp_b_dir", "验证设置结果")
        send_cmd(ser, "set motor_mp_b_dir 0", "设置电机B方向 0")
        send_cmd(ser, "get motor_mp_b_dir", "验证设置结果")
        send_cmd(ser, "set motor_mp_b_dir 2", "测试越界值 2")

        print("\n========== 10. 直接设置PWM(new_pwm_duty,不保存) ==========")
        send_cmd(ser, "get pwm_duty", "获取当前PWM")
        send_cmd(ser, "set new_pwm_duty 60", "直接设置PWM 60(不保存到Flash)")
        send_cmd(ser, "get pwm_duty", "验证当前PWM已变为60")
        # 用 set pwm_duty 恢复一个有效值，避免影响后续测试
        send_cmd(ser, "set pwm_duty 50", "恢复保存的PWM值为50")
        send_cmd(ser, "get pwm_duty", "验证恢复结果")
        send_cmd(ser, "set new_pwm_duty 101", "测试越界值 101")

        print("\n========== 11. 加速度(acceleration) ==========")
        send_cmd(ser, "get acceleration", "获取当前加速度")
        send_cmd(ser, "set acceleration 10", "设置加速度 10")
        send_cmd(ser, "get acceleration", "验证设置结果")
        send_cmd(ser, "set acceleration 0", "关闭加速度")
        send_cmd(ser, "get acceleration", "验证关闭结果")
        send_cmd(ser, "set acceleration 51", "测试越界值 51")

        print("\n========== 12. 启动信号(start_signal) ==========")
        send_cmd(ser, "get start_signal", "获取MOTOR_PM_ENABLE引脚电平")

        print("\n========== 13. 连续发送测试 ==========")
        print(">>> 连续发送测试: 5次 get pwm_duty")
        for i in range(5):
            ser.write(b"get pwm_duty\r\n")
            ser.flush()
            time.sleep(0.15)
            response = ser.read(ser.in_waiting).decode('utf-8', errors='replace')
            print(f"  第{i+1}次: {response.strip() if response else '无响应'}")
        print("-" * 50)

        print("\n========== 14. 错误命令测试 ==========")
        send_cmd(ser, "unknown_cmd", "未知命令")
        send_cmd(ser, "set", "缺少子命令")
        send_cmd(ser, "get", "缺少参数")

        print("\n========== 全部测试完成 ==========")

    except serial.SerialException as e:
        print(f"❌ 串口错误: {e}")
        print("请检查:")
        print("  1. COM18 是否存在")
        print("  2. 是否被其他程序占用")
        print("  3. 设备是否已连接")
    except Exception as e:
        print(f"❌ 错误: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print(f"\n已断开 {PORT}")


if __name__ == "__main__":
    main()
