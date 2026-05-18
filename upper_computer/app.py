import queue
import threading
import tkinter as tk
from tkinter import messagebox, ttk
from typing import Callable, Dict, Optional

from pusher_motor_serial import PusherMotorSerial, SerialCommandError, SerialPortInfo


APP_TITLE = "Pusher Motor 售后调试上位机"
DEFAULT_BAUDRATE = 115200
ACCELERATION_MAX = 50
DIRECTION_TIME_MAX_MS = 60000
WAIT_TIME_MAX_MS = 10000
SPEED_MAX_CM_MIN = 70000
WHEEL_CIRCUMFERENCE_CM = 3.14159 * 6.0
DEFAULT_DIRECTION_TIME_MS = 250
DEFAULT_WAIT_TIME_MS = 250
DEFAULT_PWM_DUTY = 150
DEFAULT_MAX_SPEED_RPM = 3655
DEFAULT_MOTOR_A_DIR = 1
DEFAULT_MOTOR_B_DIR = 0
DEFAULT_SPEED_CM_MIN = int((1.0 - DEFAULT_PWM_DUTY / 500.0) * DEFAULT_MAX_SPEED_RPM * WHEEL_CIRCUMFERENCE_CM)

COLOR_PRIMARY = "#0066cc"
COLOR_PRIMARY_FOCUS = "#1473e6"
COLOR_PRIMARY_DARK = "#2997ff"
COLOR_INK = "#1d1d1f"
COLOR_MUTED = "#7a7a7a"
COLOR_HAIRLINE = "#e0e0e0"
COLOR_CANVAS = "#ffffff"
COLOR_PARCHMENT = "#f5f5f7"
COLOR_PEARL = "#fafafc"
COLOR_DARK_TILE = "#272729"
COLOR_DARK_TILE_2 = "#2a2a2c"
FONT_UI = "Microsoft YaHei UI"
FONT_MONO = "Consolas"


class AppleSlider(tk.Canvas):
    def __init__(
        self,
        master: tk.Widget,
        variable: tk.StringVar,
        min_value: int,
        max_value: int,
        **kwargs: object,
    ) -> None:
        super().__init__(
            master,
            height=30,
            background=COLOR_CANVAS,
            highlightthickness=0,
            borderwidth=0,
            **kwargs,
        )
        self.variable = variable
        self.min_value = min_value
        self.max_value = max_value
        self._dragging = False
        self._updating = False
        self._trace_id = self.variable.trace_add("write", self._on_variable_changed)

        self.bind("<Configure>", lambda _event: self._draw())
        self.bind("<Button-1>", self._on_pointer)
        self.bind("<B1-Motion>", self._on_pointer)
        self.bind("<ButtonRelease-1>", lambda _event: setattr(self, "_dragging", False))
        self.bind("<Left>", lambda _event: self._step(-1))
        self.bind("<Right>", lambda _event: self._step(1))

    def _on_variable_changed(self, *_args: object) -> None:
        if not self._updating:
            self._draw()

    def _value(self) -> int:
        try:
            value = int(float(self.variable.get()))
        except ValueError:
            value = self.min_value
        return max(self.min_value, min(self.max_value, value))

    def _set_value(self, value: int) -> None:
        value = max(self.min_value, min(self.max_value, value))
        self._updating = True
        self.variable.set(str(value))
        self._updating = False
        self._draw()

    def _on_pointer(self, event: tk.Event) -> None:
        self.focus_set()
        self._dragging = True
        width = max(1, self.winfo_width())
        margin = 14
        usable = max(1, width - margin * 2)
        ratio = (event.x - margin) / usable
        ratio = max(0.0, min(1.0, ratio))
        value = int(round(self.min_value + ratio * (self.max_value - self.min_value)))
        self._set_value(value)

    def _step(self, direction: int) -> None:
        span = self.max_value - self.min_value
        step = max(1, span // 100)
        self._set_value(self._value() + direction * step)

    def _draw(self) -> None:
        self.delete("all")
        width = max(1, self.winfo_width())
        height = max(1, self.winfo_height())
        margin = 14
        y = height // 2
        track_left = margin
        track_right = max(track_left + 1, width - margin)
        track_width = track_right - track_left

        value = self._value()
        if self.max_value == self.min_value:
            ratio = 0.0
        else:
            ratio = (value - self.min_value) / (self.max_value - self.min_value)
        knob_x = track_left + ratio * track_width

        self.create_line(track_left, y, track_right, y, fill=COLOR_HAIRLINE, width=4)
        self.create_line(track_left, y, knob_x, y, fill=COLOR_PRIMARY, width=4)
        self.create_oval(
            knob_x - 7,
            y - 7,
            knob_x + 7,
            y + 7,
            fill=COLOR_CANVAS,
            outline=COLOR_PRIMARY,
            width=2,
        )


class FlatButton(tk.Button):
    def __init__(self, master: tk.Widget, text: str, command: Callable[[], None], primary: bool = False) -> None:
        self.primary = primary
        if primary:
            self.normal_bg = COLOR_PRIMARY
            self.hover_bg = COLOR_PRIMARY_FOCUS
            self.active_bg = COLOR_PRIMARY
            self.fg = "#ffffff"
        else:
            self.normal_bg = COLOR_CANVAS
            self.hover_bg = COLOR_PEARL
            self.active_bg = COLOR_HAIRLINE
            self.fg = COLOR_PRIMARY

        super().__init__(
            master,
            text=text,
            command=command,
            bg=self.normal_bg,
            fg=self.fg,
            activebackground=self.active_bg,
            activeforeground=self.fg,
            bd=0,
            highlightthickness=1,
            highlightbackground=(COLOR_PRIMARY if primary else COLOR_HAIRLINE),
            highlightcolor=(COLOR_PRIMARY if primary else COLOR_HAIRLINE),
            padx=14,
            pady=8,
            cursor="hand2",
            font=(FONT_UI, 10, "bold" if primary else "normal"),
            relief=tk.FLAT,
            takefocus=True,
        )
        self.bind("<Enter>", lambda _event: self.configure(bg=self.hover_bg))
        self.bind("<Leave>", lambda _event: self.configure(bg=self.normal_bg))


class ScrollablePanel(ttk.Frame):
    def __init__(self, master: tk.Widget) -> None:
        super().__init__(master, style="Root.TFrame")
        self.canvas = tk.Canvas(self, bg=COLOR_PARCHMENT, highlightthickness=0, borderwidth=0)
        self.scrollbar = ttk.Scrollbar(self, orient=tk.VERTICAL, command=self.canvas.yview)
        self.content = ttk.Frame(self.canvas, style="Root.TFrame")
        self.window_id = self.canvas.create_window((0, 0), window=self.content, anchor="nw")

        self.canvas.configure(yscrollcommand=self.scrollbar.set)
        self.canvas.grid(row=0, column=0, sticky="nsew")
        self.scrollbar.grid(row=0, column=1, sticky="ns")
        self.rowconfigure(0, weight=1)
        self.columnconfigure(0, weight=1)

        self.content.bind("<Configure>", self._on_content_configure)
        self.canvas.bind("<Configure>", self._on_canvas_configure)
        self.canvas.bind("<Enter>", self._bind_mousewheel)
        self.canvas.bind("<Leave>", self._unbind_mousewheel)

    def _on_content_configure(self, _event: tk.Event) -> None:
        self.canvas.configure(scrollregion=self.canvas.bbox("all"))

    def _on_canvas_configure(self, event: tk.Event) -> None:
        self.canvas.itemconfigure(self.window_id, width=event.width)

    def _bind_mousewheel(self, _event: tk.Event) -> None:
        self.canvas.bind_all("<MouseWheel>", self._on_mousewheel)
        self.canvas.bind_all("<Button-4>", self._on_mousewheel)
        self.canvas.bind_all("<Button-5>", self._on_mousewheel)

    def _unbind_mousewheel(self, _event: tk.Event) -> None:
        self.canvas.unbind_all("<MouseWheel>")
        self.canvas.unbind_all("<Button-4>")
        self.canvas.unbind_all("<Button-5>")

    def _on_mousewheel(self, event: tk.Event) -> None:
        if getattr(event, "num", None) == 4:
            delta = -1
        elif getattr(event, "num", None) == 5:
            delta = 1
        else:
            delta = -1 * int(event.delta / 120)
        self.canvas.yview_scroll(delta, "units")


class PusherMotorApp(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title(APP_TITLE)
        self.geometry("1040x700")
        self.minsize(940, 620)

        self.client = PusherMotorSerial()
        self.ports: Dict[str, SerialPortInfo] = {}
        self.event_queue: "queue.Queue[Callable[[], None]]" = queue.Queue()
        self.advanced_visible = False

        self.port_var = tk.StringVar()
        self.baud_var = tk.StringVar(value=str(DEFAULT_BAUDRATE))
        self.status_var = tk.StringVar(value="未连接")
        self.mode_var = tk.StringVar(value="-")

        self.direction_time_var = tk.StringVar(value=str(DEFAULT_DIRECTION_TIME_MS))
        self.wait_time_var = tk.StringVar(value=str(DEFAULT_WAIT_TIME_MS))
        self.acceleration_var = tk.StringVar(value="0")
        self.speed_var = tk.StringVar(value=str(DEFAULT_SPEED_CM_MIN))
        self.pwm_duty_var = tk.StringVar(value=str(DEFAULT_PWM_DUTY))
        self.max_speed_var = tk.StringVar(value=str(DEFAULT_MAX_SPEED_RPM))
        self.motor_a_var = tk.StringVar(value=str(DEFAULT_MOTOR_A_DIR))
        self.motor_b_var = tk.StringVar(value=str(DEFAULT_MOTOR_B_DIR))
        self.start_signal_var = tk.StringVar(value="-")

        self._build_style()
        self._build_layout()
        self.refresh_ports()
        self.after(80, self._process_events)
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    def _build_style(self) -> None:
        self.configure(bg=COLOR_PARCHMENT)
        style = ttk.Style(self)
        if "clam" in style.theme_names():
            style.theme_use("clam")
        style.configure(".", font=(FONT_UI, 10))
        style.configure("TFrame", background=COLOR_PARCHMENT)
        style.configure("Root.TFrame", background=COLOR_PARCHMENT)
        style.configure("Header.TFrame", background="#000000")
        style.configure("HeaderStatus.TFrame", background="#000000")
        style.configure("TLabel", background=COLOR_PARCHMENT, foreground=COLOR_INK, font=(FONT_UI, 10))
        style.configure("Header.TLabel", background="#000000", foreground="#ffffff", font=(FONT_UI, 18, "bold"))
        style.configure("HeaderStatus.TLabel", background="#000000", foreground=COLOR_PARCHMENT, font=(FONT_UI, 10, "bold"))
        style.configure("HeaderMode.TLabel", background="#000000", foreground=COLOR_PRIMARY_DARK, font=(FONT_UI, 10, "bold"))
        style.configure("Status.TLabel", background=COLOR_CANVAS, foreground=COLOR_INK, font=(FONT_UI, 10, "bold"))
        style.configure("Field.TLabel", background=COLOR_CANVAS, foreground=COLOR_INK, font=(FONT_UI, 10))
        style.configure("Muted.TLabel", background=COLOR_CANVAS, foreground=COLOR_MUTED, font=(FONT_UI, 9))

        style.configure(
            "TButton",
            background=COLOR_PEARL,
            foreground=COLOR_INK,
            bordercolor=COLOR_HAIRLINE,
            focusthickness=0,
            focuscolor=COLOR_PEARL,
            font=(FONT_UI, 10),
            padding=(12, 7),
            relief="flat",
        )
        style.map(
            "TButton",
            background=[("active", COLOR_CANVAS), ("pressed", COLOR_HAIRLINE)],
            foreground=[("disabled", COLOR_MUTED)],
        )
        style.configure(
            "Primary.TButton",
            background=COLOR_PRIMARY,
            foreground="#ffffff",
            bordercolor=COLOR_PRIMARY,
            focuscolor=COLOR_PRIMARY,
            font=(FONT_UI, 10, "bold"),
            padding=(15, 8),
            relief="flat",
        )
        style.map(
            "Primary.TButton",
            background=[("active", COLOR_PRIMARY_FOCUS), ("pressed", COLOR_PRIMARY)],
            foreground=[("disabled", COLOR_PEARL)],
        )
        style.configure(
            "Secondary.TButton",
            background=COLOR_CANVAS,
            foreground=COLOR_PRIMARY,
            bordercolor=COLOR_HAIRLINE,
            focuscolor=COLOR_CANVAS,
            font=(FONT_UI, 10),
            padding=(12, 7),
            relief="flat",
        )
        style.map(
            "Secondary.TButton",
            background=[("active", COLOR_PEARL), ("pressed", COLOR_HAIRLINE)],
            foreground=[("active", COLOR_PRIMARY_FOCUS)],
        )
        style.configure(
            "TEntry",
            fieldbackground=COLOR_CANVAS,
            background=COLOR_CANVAS,
            foreground=COLOR_INK,
            bordercolor=COLOR_HAIRLINE,
            lightcolor=COLOR_HAIRLINE,
            darkcolor=COLOR_HAIRLINE,
            padding=(8, 6),
            relief="flat",
        )
        style.configure(
            "TCombobox",
            fieldbackground=COLOR_CANVAS,
            background=COLOR_CANVAS,
            foreground=COLOR_INK,
            bordercolor=COLOR_HAIRLINE,
            arrowcolor=COLOR_INK,
            padding=(8, 5),
            relief="flat",
        )
        style.configure(
            "Card.TLabelframe",
            background=COLOR_CANVAS,
            bordercolor=COLOR_HAIRLINE,
            lightcolor=COLOR_CANVAS,
            darkcolor=COLOR_HAIRLINE,
            relief="flat",
            padding=(17, 14),
        )
        style.configure(
            "Card.TLabelframe.Label",
            background=COLOR_CANVAS,
            foreground=COLOR_INK,
            font=(FONT_UI, 11, "bold"),
        )
        style.configure(
            "Log.TLabelframe",
            background=COLOR_DARK_TILE,
            bordercolor=COLOR_DARK_TILE_2,
            lightcolor=COLOR_DARK_TILE,
            darkcolor=COLOR_DARK_TILE_2,
            relief="solid",
            padding=(14, 12),
        )
        style.configure(
            "Log.TLabelframe.Label",
            background=COLOR_DARK_TILE,
            foreground="#ffffff",
            font=(FONT_UI, 11, "bold"),
        )
        style.configure("LogInner.TFrame", background=COLOR_DARK_TILE)
        style.configure("Toast.TFrame", background=COLOR_CANVAS, relief="solid", borderwidth=1)
        style.configure("Toast.TLabel", background=COLOR_CANVAS, foreground=COLOR_INK, font=(FONT_UI, 11, "bold"))

    def _build_layout(self) -> None:
        root = ttk.Frame(self, padding=17, style="Root.TFrame")
        root.pack(fill=tk.BOTH, expand=True)
        root.columnconfigure(0, weight=0, minsize=360)
        root.columnconfigure(1, weight=1)
        root.rowconfigure(1, weight=1)

        header = ttk.Frame(root, padding=(17, 12), style="Header.TFrame")
        header.grid(row=0, column=0, columnspan=2, sticky="ew", pady=(0, 17))
        header.columnconfigure(0, weight=1)
        ttk.Label(header, text=APP_TITLE, style="Header.TLabel").grid(row=0, column=0, sticky="w")

        header_status = ttk.Frame(header, style="HeaderStatus.TFrame")
        header_status.grid(row=0, column=1, sticky="e")
        ttk.Label(header_status, textvariable=self.status_var, style="HeaderStatus.TLabel").grid(row=0, column=0, sticky="e")
        ttk.Label(header_status, text="  模式 ", style="HeaderStatus.TLabel").grid(row=0, column=1, sticky="e")
        ttk.Label(header_status, textvariable=self.mode_var, style="HeaderMode.TLabel").grid(row=0, column=2, sticky="e")

        left_scroll = ScrollablePanel(root)
        left_scroll.grid(row=1, column=0, sticky="nsew", padx=(0, 12))
        left = left_scroll.content
        right = ttk.Frame(root)
        right.grid(row=1, column=1, sticky="nsew")
        right.rowconfigure(0, weight=1)
        right.columnconfigure(0, weight=1)

        self._build_connection(left)
        self._build_quick_actions(left)
        self._build_runtime_params(left)
        self._build_speed_params(left)
        self._build_direction(left)
        self._build_advanced(left)
        self._build_log(right)

    def _create_card(self, parent: tk.Widget, title: str, dark: bool = False) -> tuple[tk.Frame, tk.Frame]:
        bg = COLOR_DARK_TILE if dark else COLOR_CANVAS
        border = COLOR_DARK_TILE_2 if dark else COLOR_HAIRLINE
        title_fg = "#ffffff" if dark else COLOR_INK

        outer = tk.Frame(parent, bg=bg, highlightbackground=border, highlightthickness=1, bd=0)
        tk.Label(
            outer,
            text=title,
            bg=bg,
            fg=title_fg,
            font=(FONT_UI, 11, "bold"),
            anchor="w",
        ).pack(fill=tk.X, padx=17, pady=(14, 8))
        body = tk.Frame(outer, bg=bg)
        body.pack(fill=tk.BOTH, expand=True, padx=17, pady=(0, 14))
        return outer, body

    def _button(self, parent: tk.Widget, text: str, command: Callable[[], None], primary: bool = False) -> FlatButton:
        return FlatButton(parent, text=text, command=command, primary=primary)

    def _entry(self, parent: tk.Widget, variable: tk.StringVar, width: int = 16) -> tk.Entry:
        return tk.Entry(
            parent,
            textvariable=variable,
            width=width,
            bg=COLOR_CANVAS,
            fg=COLOR_INK,
            insertbackground=COLOR_INK,
            relief=tk.FLAT,
            bd=0,
            highlightthickness=1,
            highlightbackground=COLOR_HAIRLINE,
            highlightcolor=COLOR_PRIMARY,
            font=(FONT_UI, 10),
        )

    def _build_connection(self, parent: ttk.Frame) -> None:
        outer, frame = self._create_card(parent, "连接")
        outer.pack(fill=tk.X, pady=(0, 12))
        frame.columnconfigure(1, weight=1)

        ttk.Label(frame, text="串口", style="Field.TLabel").grid(row=0, column=0, sticky="w", padx=(0, 8), pady=5)
        self.port_combo = ttk.Combobox(frame, textvariable=self.port_var, state="readonly")
        self.port_combo.grid(row=0, column=1, sticky="ew", pady=5)
        self._button(frame, "刷新", self.refresh_ports).grid(row=0, column=2, padx=(8, 0), pady=5)

        ttk.Label(frame, text="波特率", style="Field.TLabel").grid(row=1, column=0, sticky="w", padx=(0, 8), pady=5)
        self._entry(frame, self.baud_var, width=12).grid(row=1, column=1, sticky="w", pady=5, ipady=6)
        self.connect_button = self._button(frame, "连接", self.toggle_connection, primary=True)
        self.connect_button.grid(row=1, column=2, sticky="ew", padx=(8, 0), pady=5)

    def _build_quick_actions(self, parent: ttk.Frame) -> None:
        outer, frame = self._create_card(parent, "常用动作")
        outer.pack(fill=tk.X, pady=(0, 12))
        frame.columnconfigure((0, 1), weight=1)

        self._button(frame, "读取参数", self.refresh_all).grid(row=0, column=0, sticky="ew", padx=(0, 6), pady=5)
        self._button(frame, "单次运行测试", self.start_motor, primary=True).grid(row=0, column=1, sticky="ew", padx=(6, 0), pady=5)
        self._button(frame, "读取启动信号", self.read_start_signal).grid(row=1, column=0, columnspan=2, sticky="ew", pady=5)
        ttk.Label(frame, text="启动信号", style="Field.TLabel").grid(row=2, column=0, sticky="w", pady=(8, 0))
        ttk.Label(frame, textvariable=self.start_signal_var, style="Status.TLabel").grid(row=2, column=1, sticky="e", pady=(6, 0))

    def _build_runtime_params(self, parent: ttk.Frame) -> None:
        outer, frame = self._create_card(parent, "运行参数")
        outer.pack(fill=tk.X, pady=(0, 12))
        for col in range(3):
            frame.columnconfigure(col, weight=1)

        self._add_slider_param_row(
            frame,
            0,
            "运行时间 ms",
            self.direction_time_var,
            1,
            DIRECTION_TIME_MAX_MS,
            "保存",
            lambda: self.save_param("direction_time", self.direction_time_var, 1, DIRECTION_TIME_MAX_MS),
            f"工程默认值：{DEFAULT_DIRECTION_TIME_MS} ms",
        )
        self._add_slider_param_row(
            frame,
            3,
            "等待时间 ms",
            self.wait_time_var,
            0,
            WAIT_TIME_MAX_MS,
            "保存",
            lambda: self.save_param("wait_time", self.wait_time_var, 0, WAIT_TIME_MAX_MS),
            f"工程默认值：{DEFAULT_WAIT_TIME_MS} ms",
        )

    def _build_speed_params(self, parent: ttk.Frame) -> None:
        outer, frame = self._create_card(parent, "速度参数")
        outer.pack(fill=tk.X, pady=(0, 12))
        for col in range(4):
            frame.columnconfigure(col, weight=1)

        self._add_slider_param_row_two_buttons(
            frame,
            0,
            "速度 cm/min",
            self.speed_var,
            0,
            SPEED_MAX_CM_MIN,
            "应用",
            self.apply_speed,
            "保存",
            self.save_speed,
            f"工程默认值：约 {DEFAULT_SPEED_CM_MIN} cm/min（由默认 PWM {DEFAULT_PWM_DUTY} 换算）",
        )
        self._add_slider_param_row(
            frame,
            3,
            "加速度 0-50",
            self.acceleration_var,
            0,
            ACCELERATION_MAX,
            "保存",
            self.save_acceleration,
            "工程默认值：0（0=关闭加速，1-50=每10ms步长）",
        )

    def _build_direction(self, parent: ttk.Frame) -> None:
        outer, frame = self._create_card(parent, "方向")
        outer.pack(fill=tk.X, pady=(0, 12))
        frame.columnconfigure((1, 3), weight=1)

        ttk.Label(frame, text="电机A", style="Field.TLabel").grid(row=0, column=0, sticky="w", padx=(0, 6), pady=5)
        ttk.Combobox(frame, textvariable=self.motor_a_var, values=("0", "1"), state="readonly", width=6).grid(row=0, column=1, sticky="w", pady=5)
        self._button(frame, "保存A", lambda: self.save_param("motor_mp_a_dir", self.motor_a_var, 0, 1), primary=True).grid(row=0, column=2, sticky="ew", padx=(10, 0), pady=5)
        self._add_note(frame, 1, f"工程默认值：{DEFAULT_MOTOR_A_DIR}", columnspan=3)

        ttk.Label(frame, text="电机B", style="Field.TLabel").grid(row=2, column=0, sticky="w", padx=(0, 6), pady=5)
        ttk.Combobox(frame, textvariable=self.motor_b_var, values=("0", "1"), state="readonly", width=6).grid(row=2, column=1, sticky="w", pady=5)
        self._button(frame, "保存B", lambda: self.save_param("motor_mp_b_dir", self.motor_b_var, 0, 1), primary=True).grid(row=2, column=2, sticky="ew", padx=(10, 0), pady=5)
        self._add_note(frame, 3, f"工程默认值：{DEFAULT_MOTOR_B_DIR}", columnspan=3)

    def _build_advanced(self, parent: ttk.Frame) -> None:
        self.advanced_button = self._button(parent, "高级调试", self.toggle_advanced)
        self.advanced_button.pack(fill=tk.X, pady=(0, 12))

        self.advanced_outer, self.advanced_frame = self._create_card(parent, "高级调试")
        for col in range(4):
            self.advanced_frame.columnconfigure(col, weight=1)

        self._add_param_row_two_buttons(self.advanced_frame, 0, "PWM 0-500", self.pwm_duty_var, "应用", self.apply_pwm, "保存", self.save_pwm)
        self._add_note(self.advanced_frame, 1, f"工程默认值：{DEFAULT_PWM_DUTY}（范围 0-500）")
        self._add_param_row(self.advanced_frame, 2, "速度校准 RPM", self.max_speed_var, "保存", lambda: self.save_param("max_speed", self.max_speed_var, 1, 10000))
        self._add_note(self.advanced_frame, 3, f"工程默认值：{DEFAULT_MAX_SPEED_RPM} RPM，用于速度和 PWM 换算")

    def _build_log(self, parent: ttk.Frame) -> None:
        outer, frame = self._create_card(parent, "通信日志", dark=True)
        outer.grid(row=0, column=0, sticky="nsew")
        outer.rowconfigure(0, weight=0)
        outer.rowconfigure(1, weight=1)
        outer.columnconfigure(0, weight=1)
        frame.rowconfigure(0, weight=1)
        frame.columnconfigure(0, weight=1)

        self.log_text = tk.Text(
            frame,
            height=20,
            wrap=tk.WORD,
            font=(FONT_MONO, 10),
            undo=False,
            background=COLOR_DARK_TILE,
            foreground="#f5f5f7",
            insertbackground="#ffffff",
            selectbackground=COLOR_PRIMARY,
            selectforeground="#ffffff",
            relief=tk.FLAT,
            borderwidth=0,
            padx=12,
            pady=12,
        )
        self.log_text.grid(row=0, column=0, sticky="nsew")
        scrollbar = ttk.Scrollbar(frame, command=self.log_text.yview)
        scrollbar.grid(row=0, column=1, sticky="ns")
        self.log_text.configure(yscrollcommand=scrollbar.set)

        buttons = tk.Frame(frame, bg=COLOR_DARK_TILE)
        buttons.grid(row=1, column=0, columnspan=2, sticky="ew", pady=(8, 0))
        buttons.columnconfigure(0, weight=1)
        self._button(buttons, "清空日志", lambda: self.log_text.delete("1.0", tk.END)).grid(row=0, column=1, sticky="e")

    def _add_param_row(self, frame: tk.Widget, row: int, label: str, var: tk.StringVar, button_text: str, command: Callable[[], None]) -> None:
        ttk.Label(frame, text=label, style="Field.TLabel").grid(row=row, column=0, sticky="w", padx=(0, 8), pady=5)
        self._entry(frame, var, width=16).grid(row=row, column=1, sticky="ew", pady=5, ipady=6)
        self._button(frame, button_text, command, primary=True).grid(row=row, column=2, columnspan=2, sticky="ew", padx=(8, 0), pady=5)

    def _add_note(self, frame: tk.Widget, row: int, text: str, columnspan: int = 4) -> None:
        ttk.Label(frame, text=text, style="Muted.TLabel").grid(
            row=row,
            column=0,
            columnspan=columnspan,
            sticky="w",
            pady=(0, 8),
        )

    def _add_slider_param_row(
        self,
        frame: tk.Widget,
        row: int,
        label: str,
        var: tk.StringVar,
        min_value: int,
        max_value: int,
        button_text: str,
        command: Callable[[], None],
        note: str,
    ) -> None:
        self._add_param_row(frame, row, label, var, button_text, command)
        self._add_slider(frame, row + 1, var, min_value, max_value)
        self._add_note(frame, row + 2, note)

    def _add_param_row_two_buttons(
        self,
        frame: tk.Widget,
        row: int,
        label: str,
        var: tk.StringVar,
        first_text: str,
        first_command: Callable[[], None],
        second_text: str,
        second_command: Callable[[], None],
    ) -> None:
        ttk.Label(frame, text=label, style="Field.TLabel").grid(row=row, column=0, sticky="w", padx=(0, 8), pady=5)
        self._entry(frame, var, width=16).grid(row=row, column=1, sticky="ew", pady=5, ipady=6)
        self._button(frame, first_text, first_command).grid(row=row, column=2, sticky="ew", padx=(8, 4), pady=5)
        self._button(frame, second_text, second_command, primary=True).grid(row=row, column=3, sticky="ew", padx=(4, 0), pady=5)

    def _add_slider_param_row_two_buttons(
        self,
        frame: tk.Widget,
        row: int,
        label: str,
        var: tk.StringVar,
        min_value: int,
        max_value: int,
        first_text: str,
        first_command: Callable[[], None],
        second_text: str,
        second_command: Callable[[], None],
        note: str,
    ) -> None:
        self._add_param_row_two_buttons(frame, row, label, var, first_text, first_command, second_text, second_command)
        self._add_slider(frame, row + 1, var, min_value, max_value)
        self._add_note(frame, row + 2, note)

    def _add_slider(self, frame: tk.Widget, row: int, var: tk.StringVar, min_value: int, max_value: int) -> None:
        slider = AppleSlider(frame, var, min_value, max_value)
        slider.grid(row=row, column=0, columnspan=4, sticky="ew", pady=(2, 10))

    def toggle_advanced(self) -> None:
        if self.advanced_visible:
            self.advanced_outer.pack_forget()
            self.advanced_visible = False
            return

        self.advanced_outer.pack(fill=tk.X, pady=(0, 10), after=self.advanced_button)
        self.advanced_visible = True

    def refresh_ports(self) -> None:
        ports = self.client.list_ports()
        self.ports = {port.label: port for port in ports}
        labels = list(self.ports.keys())
        self.port_combo["values"] = labels
        if labels and not self.port_var.get():
            self.port_var.set(labels[0])
        if not labels:
            self.port_var.set("")
        self._log("已刷新串口列表")

    def toggle_connection(self) -> None:
        if self.client.is_connected:
            self.client.disconnect()
            self.status_var.set("未连接")
            self.mode_var.set("-")
            self.connect_button.configure(text="连接")
            self._log("已断开连接")
            return

        label = self.port_var.get()
        if label not in self.ports:
            messagebox.showerror("连接失败", "请选择有效串口")
            return

        try:
            baudrate = int(self.baud_var.get())
        except ValueError:
            messagebox.showerror("连接失败", "波特率必须是数字")
            return

        port = self.ports[label].device
        self._run_serial_task(
            "连接设备",
            lambda: self.client.connect(port, baudrate),
            on_success=lambda response: self._on_connected(port, response),
        )

    def _on_connected(self, port: str, response: str) -> None:
        self.status_var.set(f"已连接 {port}")
        self.connect_button.configure(text="断开")
        if response.strip():
            self._log(response.strip())
        self.refresh_all()

    def refresh_all(self) -> None:
        commands = [
            ("direction_time", self.direction_time_var, "get direction_time"),
            ("wait_time", self.wait_time_var, "get wait_time"),
            ("pwm_duty", self.pwm_duty_var, "get pwm_duty"),
            ("max_speed", self.max_speed_var, "get max_speed"),
            ("speed", self.speed_var, "get speed"),
            ("motor_mp_a_dir", self.motor_a_var, "get motor_mp_a_dir"),
            ("motor_mp_b_dir", self.motor_b_var, "get motor_mp_b_dir"),
        ]

        def work() -> Dict[str, object]:
            values: Dict[str, object] = {name: self.client.get_value(command) for name, _var, command in commands}
            values["acceleration"] = self.client.get_value("get acceleration")
            return values

        def done(values: Dict[str, object]) -> None:
            for name, var, _command in commands:
                var.set(str(values[name]))
            self.acceleration_var.set(str(values["acceleration"]))
            self._log("参数读取完成")

        self._run_serial_task("读取参数", work, done)

    def start_motor(self) -> None:
        def work() -> str:
            return self._send_checked_worker("start")

        def done(response: str) -> None:
            self._log(response)

        self._run_serial_task("单次运行测试", work, done)

    def read_start_signal(self) -> None:
        def work() -> str:
            return self.client.send_command("get start_signal")

        def done(response: str) -> None:
            self._log(response)
            if "HIGH" in response:
                self.start_signal_var.set("HIGH")
            elif "LOW" in response:
                self.start_signal_var.set("LOW")
            else:
                self.start_signal_var.set(response.strip() or "-")

        self._run_serial_task("读取启动信号", work, done)

    def apply_speed(self) -> None:
        speed = self._read_int(self.speed_var, 0, None)
        if speed is None:
            return

        def work() -> str:
            max_speed = self._read_max_speed_worker()
            duty = self._calculate_duty_from_speed(speed, max_speed)
            return self._send_checked_worker(f"set new_pwm_duty {duty}")

        def done(response: str) -> None:
            self._log(response)

        self._run_serial_task("应用速度", work, done)

    def save_speed(self) -> None:
        speed = self._read_int(self.speed_var, 0, None)
        if speed is None:
            return
        self._save_command(f"set speed {speed}")

    def save_acceleration(self) -> None:
        level = self._read_int(self.acceleration_var, 0, ACCELERATION_MAX)
        if level is None:
            return
        self._save_command(f"set acceleration {level}")

    def apply_pwm(self) -> None:
        duty = self._read_int(self.pwm_duty_var, 0, 500)
        if duty is None:
            return

        def work() -> str:
            return self._send_checked_worker(f"set new_pwm_duty {duty}")

        def done(response: str) -> None:
            self._log(response)

        self._run_serial_task("应用PWM", work, done)

    def save_pwm(self) -> None:
        duty = self._read_int(self.pwm_duty_var, 0, 500)
        if duty is None:
            return
        self._save_command(f"set pwm_duty {duty}")

    def save_param(self, name: str, var: tk.StringVar, min_value: int, max_value: Optional[int]) -> None:
        value = self._read_int(var, min_value, max_value)
        if value is None:
            return
        self._save_command(f"set {name} {value}")

    def _save_command(self, command: str) -> None:
        def work() -> str:
            return self._send_checked_worker(command)

        def done(response: str) -> None:
            self._log(response)
            self._show_toast("保存成功")

        self._run_serial_task("保存参数", work, done)

    def _read_int(self, var: tk.StringVar, min_value: int, max_value: Optional[int]) -> Optional[int]:
        try:
            value = int(var.get())
        except ValueError:
            messagebox.showerror("参数错误", "请输入整数")
            return None

        if value < min_value or (max_value is not None and value > max_value):
            if max_value is None:
                messagebox.showerror("参数错误", f"取值范围: >= {min_value}")
            else:
                messagebox.showerror("参数错误", f"取值范围: {min_value}-{max_value}")
            return None

        return value

    def _run_serial_task(self, title: str, work: Callable[[], object], on_success: Optional[Callable[[object], None]] = None) -> None:
        if title != "连接设备" and not self.client.is_connected:
            messagebox.showerror("串口未连接", "请先连接设备")
            return

        self.status_var.set(f"{title}...")

        def worker() -> None:
            try:
                result = work()
            except SerialCommandError as exc:
                message = str(exc)
                self.event_queue.put(lambda message=message: self._on_task_error(title, message))
            except Exception as exc:
                message = str(exc)
                self.event_queue.put(lambda message=message: self._on_task_error(title, message))
            else:
                self.event_queue.put(lambda: self._on_task_success(title, result, on_success))

        threading.Thread(target=worker, daemon=True).start()

    def _on_task_success(self, title: str, result: object, on_success: Optional[Callable[[object], None]]) -> None:
        if self.client.is_connected:
            self.status_var.set(f"已连接 {self.client.port_name}")
        else:
            self.status_var.set("未连接")
        if on_success is not None:
            on_success(result)
        elif result:
            self._log(str(result))
        self._log(f"{title} 完成")

    def _on_task_error(self, title: str, message: str) -> None:
        if self.client.is_connected:
            self.status_var.set(f"已连接 {self.client.port_name}")
        else:
            self.status_var.set("未连接")
            self.connect_button.configure(text="连接")
        self._log(f"{title} 失败: {message}")
        messagebox.showerror(title, message)

    def _process_events(self) -> None:
        while True:
            try:
                callback = self.event_queue.get_nowait()
            except queue.Empty:
                break
            callback()
        self.after(80, self._process_events)

    def _send_checked_worker(self, command: str) -> str:
        response = self.client.send_command(command)
        self._raise_on_device_error(response)
        return response

    def _read_max_speed_worker(self) -> int:
        try:
            value = int(self.max_speed_var.get())
            if value > 0:
                return value
        except ValueError:
            pass
        return self.client.get_value("get max_speed")

    @staticmethod
    def _calculate_duty_from_speed(speed_cm_min: int, max_speed_rpm: int) -> int:
        speed_rpm = float(speed_cm_min) / WHEEL_CIRCUMFERENCE_CM
        if speed_rpm <= 0:
            return 500
        if speed_rpm >= max_speed_rpm:
            return 0
        duty = int(((float(max_speed_rpm) - speed_rpm) / float(max_speed_rpm)) * 500.0)
        return max(0, min(500, duty))

    @staticmethod
    def _raise_on_device_error(response: str) -> None:
        for raw_line in response.splitlines():
            line = raw_line.strip()
            if line.startswith("Error:") or line.startswith("Unknown command"):
                raise SerialCommandError(line)

    def _show_toast(self, message: str) -> None:
        toast = tk.Toplevel(self)
        toast.overrideredirect(True)
        toast.attributes("-topmost", True)
        toast.configure(bg=COLOR_CANVAS)
        frame = ttk.Frame(toast, padding=(22, 14), style="Toast.TFrame")
        frame.pack(fill=tk.BOTH, expand=True)
        ttk.Label(frame, text=message, style="Toast.TLabel").pack()
        self.update_idletasks()
        width = toast.winfo_reqwidth()
        height = toast.winfo_reqheight()
        x = self.winfo_rootx() + (self.winfo_width() - width) // 2
        y = self.winfo_rooty() + 80
        toast.geometry(f"{width}x{height}+{x}+{y}")
        toast.after(1000, toast.destroy)

    def _log(self, message: str) -> None:
        message = message.strip()
        if not message:
            return
        self.log_text.insert(tk.END, message + "\n")
        self.log_text.see(tk.END)

    def _on_close(self) -> None:
        self.client.disconnect()
        self.destroy()


if __name__ == "__main__":
    app = PusherMotorApp()
    app.mainloop()
