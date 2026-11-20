# SPDX-License-Identifier: GPL-3.0-or-later
"""Read Windows joystick axes through WinMM for a short hardware check."""

from __future__ import annotations

import argparse
import ctypes
import time
from ctypes import wintypes


class JOYINFOEX(ctypes.Structure):
    _fields_ = [
        ("dwSize", wintypes.DWORD),
        ("dwFlags", wintypes.DWORD),
        ("dwXpos", wintypes.DWORD),
        ("dwYpos", wintypes.DWORD),
        ("dwZpos", wintypes.DWORD),
        ("dwRpos", wintypes.DWORD),
        ("dwUpos", wintypes.DWORD),
        ("dwVpos", wintypes.DWORD),
        ("dwButtons", wintypes.DWORD),
        ("dwButtonNumber", wintypes.DWORD),
        ("dwPOV", wintypes.DWORD),
        ("dwReserved1", wintypes.DWORD),
        ("dwReserved2", wintypes.DWORD),
    ]


JOY_RETURNALL = 0x000000FF
JOYERR_NOERROR = 0


def read_axis(winmm, index: int) -> tuple[int, ...] | None:
    info = JOYINFOEX()
    info.dwSize = ctypes.sizeof(JOYINFOEX)
    info.dwFlags = JOY_RETURNALL
    result = winmm.joyGetPosEx(index, ctypes.byref(info))
    if result != JOYERR_NOERROR:
        return None
    return (
        info.dwXpos,
        info.dwYpos,
        info.dwZpos,
        info.dwRpos,
        info.dwUpos,
        info.dwVpos,
        info.dwPOV,
        info.dwButtons,
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--index", type=int, default=-1)
    parser.add_argument("--seconds", type=float, default=20.0)
    args = parser.parse_args()

    winmm = ctypes.WinDLL("winmm")
    devices = []
    for index in range(16):
        axes = read_axis(winmm, index)
        if axes is not None:
            devices.append((index, axes))
    print("devices:", [index for index, _ in devices], flush=True)
    if args.index >= 0:
        devices = [(args.index, read_axis(winmm, args.index))]

    start = time.time()
    previous = None
    while time.time() - start < args.seconds:
        rows = []
        for index, _ in devices:
            axes = read_axis(winmm, index)
            if axes is not None:
                rows.append((index, axes))
        print(rows, flush=True)
        time.sleep(0.5)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
