// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <stdint.h>

namespace cfg {

// Default: XH-S3E-AI_V1.0, tested with IO41/SDA and IO42/SCL.
// The optional XIAO profile compiles with -DWEBFPV_BOARD_XIAO.
#ifdef WEBFPV_BOARD_XIAO
constexpr uint8_t PIN_SDA = 8;
constexpr uint8_t PIN_SCL = 9;
#else
constexpr uint8_t PIN_SDA = 41;
constexpr uint8_t PIN_SCL = 42;
#endif
constexpr uint8_t MPU6050_ADDRESS = 0x68;

// The DevKit BOOT button. It re-levels the controller when held for 1.2 s.
constexpr uint8_t PIN_RECENTER = 0;
constexpr uint32_t RECENTER_HOLD_MS = 1200;

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t SAMPLE_MS = 10;
constexpr uint32_t SAMPLE_US = SAMPLE_MS * 1000;

constexpr float ACCEL_LSB_PER_G = 4096.0f;
constexpr float GYRO_LSB_PER_DPS = 65.5f;
constexpr float GRAVITY = 9.80665f;

constexpr float MAX_TILT_DEG = 35.0f;
constexpr float TILT_DEADZONE_DEG = 3.0f;
constexpr float TILT_EXPO = 0.35f;

constexpr float YAW_MAX_RATE_DPS = 180.0f;
constexpr float YAW_DEADZONE_DPS = 8.0f;
constexpr float YAW_EXPO = 0.25f;

constexpr bool INVERT_ROLL = false;
constexpr bool INVERT_PITCH = false;
constexpr bool INVERT_YAW = false;

constexpr uint16_t CAL_SAMPLES = 200;
constexpr uint16_t CAL_SAMPLE_MS = 5;
constexpr float COMPLEMENTARY_ALPHA = 0.98f;

constexpr uint32_t I2C_TIMEOUT_MS = 30;
constexpr uint8_t I2C_FAILURES_TO_NEUTRAL = 3;
constexpr uint32_t SENSOR_RETRY_MS = 1000;

constexpr const char* USB_PRODUCT_NAME = "WebFPV Tilt Pad";
constexpr const char* USB_MANUFACTURER = "WebFPV Classroom";

}  // namespace cfg
