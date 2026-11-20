# WebFPV Tilt Pad

ESP32-S3 firmware and wiring for a USB motion controller for this WebFPV fork.
Roll and pitch follow tilt, yaw follows angular rate, and **W/S control throttle**.
The firmware and tools in this folder are GPL-3.0-or-later, under the repository
[LICENSE](../../LICENSE). This is a simulator input device.

## Hardware and wiring

Use an ESP32-S3 board with its native USB port and an MPU6050-compatible I2C module.
The default build targets **XH-S3E-AI_V1.0, 16 MB Flash**. An optional profile targets
an **8 MB XIAO ESP32S3**. Confirm your own board capacity before building or flashing.

Disconnect USB before changing wires. Connect the module to the board's **3V3** rail.

| Module pin | XH-S3E-AI_V1.0, default | XIAO ESP32S3, optional |
|---|---|---|
| VCC | 3V3 | 3V3 |
| GND | GND | GND |
| SCL | IO42 / SCL | D10 / GPIO9 |
| SDA | IO41 / SDA | D9 / GPIO8 |
| AD0 | GND, address 0x68 | GND, address 0x68 |
| XDA, XCL, INT | Leave unconnected | Leave unconnected |

Use soldered headers and firm jumper connections. Select the native ESP32-S3 USB
port, rather than a USB-to-UART port, for HID. See [wiring and calibration](docs/WIRING.md).

## Build, without flashing

Tested toolchain: Arduino CLI with **Arduino-ESP32 2.0.17**. Install that version
using Arduino IDE Boards Manager, or with these Arduino CLI commands:

```powershell
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@2.0.17
```

From the repository root:

```powershell
# Default XH-S3E, 16 MB. Also runs the WebFPV input regression checks.
.\hardware\tilt-controller\scripts\verify.ps1
# Optional XIAO pin/Flash profile. Compilation does not establish sensor operation.
.\hardware\tilt-controller\scripts\verify.ps1 -Board xiao -SkipWebFPV
# If the CLI is elsewhere:
.\hardware\tilt-controller\scripts\verify.ps1 -ArduinoCli '<path-to-arduino-cli.exe>'
```

The script discovers Arduino CLI on PATH or in the standard Arduino IDE install.
It stages the production mapping header beside its test sketch and compiles 11
static assertions. No serial port is opened and no board is flashed.
Build artifacts stay under `.build/<board>/firmware/`, excluded from Git.
The build selects USB-OTG/TinyUSB and CDC diagnostics. A Hardware CDC/JTAG-only
USB configuration cannot provide this gamepad interface.

For the exact FQBNs and alternate pin define, see [verify.ps1](scripts/verify.ps1).
Firmware configuration is [config.h](firmware/webfpv_tilt_pad/config.h).

## Flash a board deliberately

Back up its original Flash privately first, verify the backup and confirm the
chip, Flash capacity, USB mode and pinout. This repository does not contain any
original vendor firmware or user device backups. The verify script never uploads.

For a compatible board, Arduino CLI can upload the compiled sketch:

```powershell
arduino-cli upload --port <port> --fqbn <the-same-fqbn-used-to-build> --input-dir <build-firmware-directory>
```

Use the real current port and the same FQBN as the build. If automatic entry into
download mode fails, hold IO0/BOOT, briefly press RST, then release IO0 and check
which port appeared. After upload, release IO0 and reset into application mode.
Verify that both the USB gamepad and serial diagnostics are present before flying.

## Use WebFPV

1. Run `./start-local.ps1` from the repository root and open the printed local URL.
2. Leave the sensor still while the firmware collects its startup zero.
3. If the zero is wrong, keep the controller in your intended neutral pose,
   hold **IO0/BOOT for 3 seconds**, release it, then keep still for 2 more seconds.
   The firmware triggers after 1.2 seconds and samples for about one second.
   Level alone does not replace an earlier zero. Do not hold BOOT during power-on.
4. Select the USB controller in Settings > Choose joystick. Windows may display
   `TinyUSB HID (Vendor: 303a Product: 1001)` instead of `WebFPV Tilt Pad`.
5. Settings > Check sticks shows the mapping. For a reversed direction, tilt and
   hold just that axis, press **R** or its **Reverse** button, return to neutral,
   then **Save mapping**. Escape cancels the draft. Reversal depends on mounting.
6. Fly: tilt left/right for roll, forward/back for pitch, twist for yaw rate.
   **W raises throttle, S lowers it, R resets, Esc pauses.**

Windows raw HID tested here exposes 10 axes and 32 buttons. Its mapping is
**yaw=0, roll=2, pitch=5**. An unrelated high constant on axis 9 is ignored.
For the product-name/standard-layout path pitch uses axis 3. Saved mappings are
scoped to that layout and to the browser's local site storage, so another browser
or hostname starts with its own settings. Posture is never assigned to throttle.

Releasing W/S after takeoff follows the existing keyboard hover-throttle behavior,
which is not automatic altitude hold. In Acro mode a neutral roll/pitch input
stops commanded rotation; it does not automatically level an already tilted quad.

## Validation and limits

On 2026-11-20 an XH-S3E board with IO41/42 delivered live sensor data and USB HID
input on Windows/Edge. After a stationary IO0 recalibration, six status samples
in 12 seconds all reported zero HID axes; a separate 4-second Windows axis read
also stayed centered. The user confirmed simulator control and subsequently
confirmed the saved roll reversal worked. Pitch direction, yaw feel and full
travel still need checking for each physical mounting.

The tested module was labeled MPU-6050 but reported **WHO_AM_I=0x70**. This is an
observation, not identification as a standard MPU6050 or a general compatibility
claim. Firmware accepts nonzero identity values and uses a compatible register
layout; validate the sensor fitted to your own module. The optional XIAO profile
has a compilation check, without a completed sensor or flight acceptance test.

Read serial diagnostics at 115200 baud. A native CDC host must assert DTR.
Periodic `TILT` and `REF` lines fit the 64-byte transmit FIFO of the tested core.
`REF z=` is the saved zero, `a=` is the current accelerometer angle, and `H=` in
`TILT` is roll/pitch/yaw HID output. An `ok` line alone is not a calibration test.
For Windows raw axes, run `py -3 hardware/tilt-controller/scripts/read_gamepad_axes.py --seconds 5`.

The firmware reports neutral after repeated I2C read failures and retries a lost
sensor. Calibration is volatile and runs again after reset. The simple angle
filter is intended for modest hand tilts, not continuous inverted rotation; the
roll angle boundary at +/-180 degrees has not been validated.

## Source references

- [Arduino-ESP32 2.0.17 USB gamepad implementation](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/USB/src/USBHIDGamepad.cpp)
- [Arduino-ESP32 2.0.17 XIAO pin definitions](https://github.com/espressif/arduino-esp32/blob/2.0.17/variants/XIAO_ESP32S3/pins_arduino.h)
- [ESP32-S3 boot mode selection](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/advanced-topics/boot-mode-selection.html)
