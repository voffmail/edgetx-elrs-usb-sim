#!/usr/bin/env python3
"""Live view of the gamepad axes and buttons on Linux.

Usage:
    python3 tools/jsmon.py [/dev/input/js0]

Quit with Ctrl+C. Reads the joystick device directly, no extra packages
and no root needed (the js* node is world-readable on most systems).
"""

import glob
import os
import select
import struct
import sys

EVENT = struct.Struct("<IhBB")
JS_EVENT_BUTTON = 0x01
JS_EVENT_AXIS = 0x02

AXIS_NAMES = ["X", "Y", "Z", "Rx", "Ry", "Rz", "Throttle", "Rudder",
              "Wheel", "Gas", "Brake", "Hat0X", "Hat0Y"]
BAR_WIDTH = 25
FULL_SCALE = 32767


def find_device(argv):
    if len(argv) > 1:
        return argv[1]
    nodes = sorted(glob.glob("/dev/input/js*"))
    if not nodes:
        sys.exit("Нет /dev/input/js*: плата не подключена?")
    return nodes[0]


def device_name(path):
    sysname = os.path.basename(os.path.realpath(path))
    try:
        with open(f"/sys/class/input/{sysname}/device/name") as f:
            return f.read().strip()
    except OSError:
        return os.path.basename(path)


def bar(value):
    pos = int(round((value + FULL_SCALE) / (2 * FULL_SCALE) * (BAR_WIDTH - 1)))
    cells = ["-"] * BAR_WIDTH
    cells[BAR_WIDTH // 2] = "|"
    cells[max(0, min(BAR_WIDTH - 1, pos))] = "#"
    return "".join(cells)


def render(name, axes, buttons):
    lines = [f"{name}", ""]
    for i in sorted(axes):
        label = AXIS_NAMES[i] if i < len(AXIS_NAMES) else f"A{i}"
        lines.append(f"{label:>8} [{bar(axes[i])}] {axes[i]:>7}")
    pressed = "".join("#" if buttons.get(i) else "." for i in range(12))
    lines.append("")
    lines.append("Buttons 1-12: " + pressed)
    lines.append("")
    lines.append("Ctrl+C — выход")
    return "\x1b[H" + "\n".join(line.ljust(70) for line in lines) + "\x1b[J"


def main():
    path = find_device(sys.argv)
    try:
        fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    except PermissionError:
        sys.exit(f"Нет доступа к {path}. Добавься в группу input и перелогинься:\n"
                 f"  sudo usermod -aG input $USER")
    except OSError as exc:
        sys.exit(f"Не открыть {path}: {exc}")

    name = device_name(path)
    axes = {}
    buttons = {}

    sys.stdout.write("\x1b[?25l\x1b[2J")  # hide cursor, clear screen
    sys.stdout.flush()
    try:
        while True:
            select.select([fd], [], [], 0.05)
            while True:
                try:
                    data = os.read(fd, EVENT.size * 64)
                except BlockingIOError:
                    break
                if not data:
                    break
                for offset in range(0, len(data) - EVENT.size + 1, EVENT.size):
                    _time, value, kind, number = EVENT.unpack_from(data, offset)
                    if kind & JS_EVENT_AXIS:
                        axes[number] = value
                    elif kind & JS_EVENT_BUTTON:
                        buttons[number] = value
            sys.stdout.write(render(name, axes, buttons))
            sys.stdout.flush()
    except KeyboardInterrupt:
        pass
    finally:
        os.close(fd)
        sys.stdout.write("\x1b[?25h\n")
        sys.stdout.flush()


if __name__ == "__main__":
    main()
