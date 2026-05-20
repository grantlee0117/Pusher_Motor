import re
import threading
import time
from dataclasses import dataclass
from typing import List, Optional

import serial
from serial.tools import list_ports


class SerialCommandError(RuntimeError):
    pass


@dataclass(frozen=True)
class SerialPortInfo:
    device: str
    description: str

    @property
    def label(self) -> str:
        if self.description and self.description != "n/a":
            return f"{self.device} - {self.description}"
        return self.device


class PusherMotorSerial:
    def __init__(self) -> None:
        self._serial: Optional[serial.Serial] = None
        self._lock = threading.Lock()
        self._ready_probe = "get direction_time"

    @staticmethod
    def list_ports() -> List[SerialPortInfo]:
        return [
            SerialPortInfo(port.device, port.description or "")
            for port in list_ports.comports()
        ]

    @property
    def is_connected(self) -> bool:
        return self._serial is not None and self._serial.is_open

    @property
    def port_name(self) -> str:
        if self._serial is None:
            return ""
        return str(self._serial.port)

    def connect(self, port: str, baudrate: int = 115200, timeout: float = 0.08) -> str:
        self.disconnect()
        serial_port = serial.Serial()
        serial_port.port = port
        serial_port.baudrate = baudrate
        serial_port.bytesize = serial.EIGHTBITS
        serial_port.parity = serial.PARITY_NONE
        serial_port.stopbits = serial.STOPBITS_ONE
        serial_port.timeout = timeout
        serial_port.write_timeout = 1.0
        serial_port.rtscts = False
        serial_port.dsrdtr = False
        serial_port.dtr = False
        serial_port.rts = False
        serial_port.open()
        self._serial = serial_port

        try:
            return self._wait_until_ready()
        except Exception:
            self.disconnect()
            raise

    def disconnect(self) -> None:
        if self._serial is not None:
            try:
                if self._serial.is_open:
                    self._serial.close()
            finally:
                self._serial = None

    def send_command(self, command: str, timeout: float = 2.5) -> str:
        if not self.is_connected or self._serial is None:
            raise SerialCommandError("串口未连接")

        command = command.strip()
        if not command:
            raise SerialCommandError("命令为空")

        with self._lock:
            try:
                self._drain_input()
                self._serial.write((command + "\r\n").encode("ascii"))
                self._serial.flush()
                response = self._read_until_prompt(timeout)
            except serial.SerialException as exc:
                raise SerialCommandError(str(exc)) from exc

        return self._clean_response(command, response)

    def _wait_until_ready(self, timeout: float = 3.0) -> str:
        if self._serial is None:
            return ""

        chunks = []
        deadline = time.monotonic() + timeout
        time.sleep(0.15)
        boot_text = self._drain_input()
        if boot_text:
            chunks.append(boot_text)

        last_error = ""
        while time.monotonic() < deadline:
            try:
                response = self.send_command(self._ready_probe, timeout=0.7)
            except SerialCommandError as exc:
                last_error = str(exc)
                time.sleep(0.15)
                continue

            if response.strip():
                chunks.append(response)
            return "\n".join(chunk for chunk in chunks if chunk.strip())

        if last_error:
            raise SerialCommandError(f"设备串口已打开，但没有收到有效响应: {last_error}")
        raise SerialCommandError("设备串口已打开，但没有收到有效响应")

    def get_value(self, command: str) -> int:
        response = self.send_command(command)
        match = re.search(r"(-?\d+)", response)
        if not match:
            raise SerialCommandError(f"无法解析返回值: {response.strip()}")
        return int(match.group(1))

    def _drain_input(self) -> str:
        if not self.is_connected or self._serial is None:
            return ""

        chunks = []
        end_time = time.monotonic() + 0.25
        while time.monotonic() < end_time:
            waiting = self._serial.in_waiting
            if waiting:
                chunks.append(self._serial.read(waiting))
                end_time = time.monotonic() + 0.08
            else:
                time.sleep(0.02)
        return b"".join(chunks).decode("utf-8", errors="replace")

    def _read_until_prompt(self, timeout: float) -> str:
        if self._serial is None:
            return ""

        chunks = []
        end_time = time.monotonic() + timeout
        while time.monotonic() < end_time:
            waiting = self._serial.in_waiting
            chunk = self._serial.read(waiting or 1)
            if chunk:
                chunks.append(chunk)
                text = b"".join(chunks).decode("utf-8", errors="replace")
                stripped = text.rstrip()
                if stripped.endswith(">"):
                    return text
            else:
                time.sleep(0.01)

        text = b"".join(chunks).decode("utf-8", errors="replace")
        raise SerialCommandError(f"等待设备响应超时: {text.strip()}")

    @staticmethod
    def _clean_response(command: str, response: str) -> str:
        lines = []
        command_seen = False
        for raw_line in response.replace("\r", "\n").split("\n"):
            line = raw_line.strip()
            if not line or line == ">":
                continue
            if line == command and not command_seen:
                command_seen = True
                continue
            if line.endswith(">"):
                line = line[:-1].strip()
            if line:
                lines.append(line)
        return "\n".join(lines)
