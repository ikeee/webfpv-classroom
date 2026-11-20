// SPDX-License-Identifier: GPL-3.0-or-later
#include <Arduino.h>
#include <Wire.h>

#include "USB.h"
#include "USBHIDGamepad.h"

#include "axis_map.h"
#include "config.h"

USBHIDGamepad gamepad;

namespace {

constexpr uint8_t REG_WHO_AM_I = 0x75;
constexpr uint8_t REG_PWR_MGMT_1 = 0x6B;
constexpr uint8_t REG_CONFIG = 0x1A;
constexpr uint8_t REG_GYRO_CONFIG = 0x1B;
constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;

float gyroBiasDps = 0.0f;
float gyroBiasX = 0.0f;
float gyroBiasY = 0.0f;
float gyroBiasZ = 0.0f;
float rollZero = 0.0f;
float pitchZero = 0.0f;
float rollFiltered = 0.0f;
float pitchFiltered = 0.0f;
float yawRateFiltered = 0.0f;
uint8_t mpuAddress = cfg::MPU6050_ADDRESS;

bool sensorReady = false;
uint8_t sensorFailures = 0;
uint32_t lastSampleUs = 0;
uint32_t lastRetryMs = 0;
uint32_t lastStatusMs = 0;

bool previousRecenterButton = false;
uint32_t recenterPressedAt = 0;
bool recenterHandled = false;

void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(mpuAddress);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t readRegister(uint8_t reg) {
  Wire.beginTransmission(mpuAddress);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return 0;
  }
  if (Wire.requestFrom(mpuAddress, static_cast<uint8_t>(1), static_cast<uint8_t>(true)) != 1) {
    return 0;
  }
  return Wire.available() ? Wire.read() : 0;
}

bool readMotion(int16_t accel[3], int16_t gyro[3]) {
  Wire.beginTransmission(mpuAddress);
  Wire.write(REG_ACCEL_XOUT_H);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(mpuAddress, static_cast<uint8_t>(14), static_cast<uint8_t>(true)) != 14) {
    return false;
  }

  uint8_t raw[14];
  for (uint8_t i = 0; i < sizeof(raw); ++i) {
    raw[i] = Wire.read();
  }

  accel[0] = static_cast<int16_t>((raw[0] << 8) | raw[1]);
  accel[1] = static_cast<int16_t>((raw[2] << 8) | raw[3]);
  accel[2] = static_cast<int16_t>((raw[4] << 8) | raw[5]);
  gyro[0] = static_cast<int16_t>((raw[8] << 8) | raw[9]);
  gyro[1] = static_cast<int16_t>((raw[10] << 8) | raw[11]);
  gyro[2] = static_cast<int16_t>((raw[12] << 8) | raw[13]);
  return true;
}

bool sensorBegin() {
  uint8_t foundAddress = 0;
  for (uint8_t candidate : {cfg::MPU6050_ADDRESS, static_cast<uint8_t>(0x69)}) {
    mpuAddress = candidate;
    if (readRegister(REG_WHO_AM_I) != 0x00) {
      foundAddress = candidate;
      break;
    }
  }

  if (foundAddress == 0) {
    Serial.println("MPU6050 not found at 0x68 or 0x69.");
    Serial.printf("Check 3V3/GND, SDA=GPIO%d SCL=GPIO%d, AD0.\n",
                  cfg::PIN_SDA, cfg::PIN_SCL);
    for (uint8_t address = 1; address < 127; ++address) {
      Wire.beginTransmission(address);
      if (Wire.endTransmission() == 0) {
        Serial.printf("I2C device found at 0x%02X\n", address);
      }
    }
    for (bool stop : {false, true}) {
      for (uint8_t reg : {static_cast<uint8_t>(0x75), static_cast<uint8_t>(0x6B), static_cast<uint8_t>(0x3B)}) {
        Wire.beginTransmission(0x68);
        Wire.write(reg);
        const uint8_t err = Wire.endTransmission(stop);
        const uint8_t count = err == 0 ? Wire.requestFrom(static_cast<uint8_t>(0x68), static_cast<uint8_t>(1), static_cast<uint8_t>(true)) : 0;
        const int value = Wire.available() ? Wire.read() : -1;
        Serial.printf("REG stop=%d r=%02X e=%d n=%d v=%d\n", stop, reg, err, count, value);
      }
    }
    return false;
  }

  mpuAddress = foundAddress;
  Serial.printf("MPU6050 address 0x%02X\n", mpuAddress);
  Wire.beginTransmission(mpuAddress);
  Wire.write(REG_PWR_MGMT_1);
  Wire.write(0x80);
  Wire.endTransmission();
  delay(100);

  writeRegister(REG_PWR_MGMT_1, 0x00);
  delay(50);
  writeRegister(REG_CONFIG, 0x04);       // DLPF, about 21 Hz.
  writeRegister(REG_GYRO_CONFIG, 0x08);  // +/-500 degrees per second.
  writeRegister(REG_ACCEL_CONFIG, 0x10);  // +/-8 g.

  const uint8_t whoAmI = readRegister(REG_WHO_AM_I);
  Serial.printf("MPU6050 WHO_AM_I=0x%02X\n", whoAmI);
  if (whoAmI == 0x00) {
    Serial.println("MPU6050 did not answer on I2C.");
    return false;
  }
  return true;
}

float accelAngleRoll(int16_t ax, int16_t ay, int16_t az) {
  (void)ax;
  return atan2f(static_cast<float>(ay), static_cast<float>(az)) * RAD_TO_DEG;
}

float accelAnglePitch(int16_t ax, int16_t ay, int16_t az) {
  const float horizontal = sqrtf(static_cast<float>(ay) * ay + static_cast<float>(az) * az);
  return atan2f(-static_cast<float>(ax), horizontal) * RAD_TO_DEG;
}

bool calibrateLevel() {
  Serial.println("Hold the controller still while the MPU6050 is calibrated.");
  float sumX = 0.0f;
  float sumY = 0.0f;
  float sumZ = 0.0f;
  float rollSum = 0.0f;
  float pitchSum = 0.0f;
  uint16_t valid = 0;

  for (uint16_t i = 0; i < cfg::CAL_SAMPLES; ++i) {
    int16_t accel[3] = {0};
    int16_t gyro[3] = {0};
    if (readMotion(accel, gyro)) {
      sumX += static_cast<float>(gyro[0]) / cfg::GYRO_LSB_PER_DPS;
      sumY += static_cast<float>(gyro[1]) / cfg::GYRO_LSB_PER_DPS;
      sumZ += static_cast<float>(gyro[2]) / cfg::GYRO_LSB_PER_DPS;
      rollSum += accelAngleRoll(accel[0], accel[1], accel[2]);
      pitchSum += accelAnglePitch(accel[0], accel[1], accel[2]);
      valid += 1;
    }
    delay(cfg::CAL_SAMPLE_MS);
  }

  if (valid < cfg::CAL_SAMPLES / 2) {
    Serial.println("MPU6050 calibration failed: too few valid samples.");
    return false;
  }

  gyroBiasX = sumX / valid;
  gyroBiasY = sumY / valid;
  gyroBiasZ = sumZ / valid;
  gyroBiasDps = sqrtf(gyroBiasX * gyroBiasX + gyroBiasY * gyroBiasY + gyroBiasZ * gyroBiasZ);
  rollZero = rollSum / valid;
  pitchZero = pitchSum / valid;
  rollFiltered = rollZero;
  pitchFiltered = pitchZero;
  yawRateFiltered = 0.0f;

  Serial.printf("Calibrated: gyro bias %.2f dps, level roll %.2f pitch %.2f\n",
                gyroBiasDps, rollZero, pitchZero);
  return true;
}

void sendNeutral() {
  gamepad.send(0, 0, 0, 0, 0, 0, HAT_CENTER, 0);
}

// Each periodic line fits the installed 64-byte TinyUSB CDC TX FIFO.
void printStatus(uint32_t nowMs, float roll, float pitch, float yawDps,
                 int8_t rollAxis, int8_t pitchAxis, int8_t yawAxis,
                 float accelRoll, float accelPitch) {
  if (nowMs - lastStatusMs < 1000) return;
  lastStatusMs = nowMs;
  static bool referenceLine = false;
  char line[64];
  if (referenceLine) {
    snprintf(line, sizeof(line), "REF z=%.1f,%.1f a=%.1f,%.1f id=%02X\n",
             rollZero, pitchZero, accelRoll, accelPitch, readRegister(REG_WHO_AM_I));
  } else {
    snprintf(line, sizeof(line), "TILT r=%.1f p=%.1f y=%.1f H=%d,%d,%d ok\n",
             roll, pitch, yawDps, rollAxis, pitchAxis, yawAxis);
  }
  referenceLine = !referenceLine;
  const size_t length = strlen(line);
  if (Serial.availableForWrite() >= static_cast<int>(length)) Serial.write((const uint8_t*)line, length);
}

}  // namespace

void setup() {
  USB.manufacturerName(cfg::USB_MANUFACTURER);
  USB.productName(cfg::USB_PRODUCT_NAME);
  gamepad.begin();
  USB.begin();

  Serial.begin(cfg::SERIAL_BAUD);
  Serial.setTxTimeoutMs(0);
  Serial.println();
  Serial.println("WebFPV Tilt Pad boot");

  pinMode(cfg::PIN_RECENTER, INPUT_PULLUP);

  Wire.begin(cfg::PIN_SDA, cfg::PIN_SCL);
  Wire.setClock(100000);
  Wire.setTimeOut(cfg::I2C_TIMEOUT_MS);

  sensorReady = sensorBegin();
  if (sensorReady) {
    sensorReady = calibrateLevel();
  }

  sendNeutral();
  lastSampleUs = micros();
}

void loop() {
  const uint32_t nowMs = millis();
  const bool recenterButton = digitalRead(cfg::PIN_RECENTER) == LOW;
  if (recenterButton && !previousRecenterButton) {
    recenterPressedAt = nowMs;
    recenterHandled = false;
  }
  if (recenterButton && !recenterHandled && nowMs - recenterPressedAt >= cfg::RECENTER_HOLD_MS) {
    recenterHandled = true;
    if (sensorReady) {
      calibrateLevel();
    }
  }
  previousRecenterButton = recenterButton;

  if (!sensorReady) {
    sendNeutral();
    if (nowMs - lastStatusMs >= 1000) {
      lastStatusMs = nowMs;
      if (Serial.availableForWrite() >= 60) {
        Serial.printf("MPU6050 not ready: check SDA=GPIO%d SCL=GPIO%d and AD0=GND\n",
                      cfg::PIN_SDA, cfg::PIN_SCL);
      }
    }
    if (nowMs - lastRetryMs >= cfg::SENSOR_RETRY_MS) {
      lastRetryMs = nowMs;
      sensorReady = sensorBegin() && calibrateLevel();
    }
    delay(2);
    return;
  }

  const uint32_t nowUs = micros();
  if (nowUs - lastSampleUs < cfg::SAMPLE_US) {
    delay(1);
    return;
  }

  const float dt = constrain((nowUs - lastSampleUs) / 1000000.0f, 0.001f, 0.05f);
  lastSampleUs = nowUs;

  int16_t accel[3] = {0};
  int16_t gyro[3] = {0};
  if (!readMotion(accel, gyro)) {
    sensorFailures += 1;
    if (sensorFailures >= cfg::I2C_FAILURES_TO_NEUTRAL) {
      sendNeutral();
    }
    if (sensorFailures >= 20) {
      sensorReady = false;
      lastRetryMs = nowMs;
      Serial.println("MPU6050 lost; axes centred.");
    }
    return;
  }
  sensorFailures = 0;

  const float accelRoll = accelAngleRoll(accel[0], accel[1], accel[2]);
  const float accelPitch = accelAnglePitch(accel[0], accel[1], accel[2]);
  const float gyroXDps = static_cast<float>(gyro[0]) / cfg::GYRO_LSB_PER_DPS - gyroBiasX;
  const float gyroYDps = static_cast<float>(gyro[1]) / cfg::GYRO_LSB_PER_DPS - gyroBiasY;
  const float gyroZDps = static_cast<float>(gyro[2]) / cfg::GYRO_LSB_PER_DPS - gyroBiasZ;

  rollFiltered = cfg::COMPLEMENTARY_ALPHA * (rollFiltered + gyroXDps * dt)
      + (1.0f - cfg::COMPLEMENTARY_ALPHA) * accelRoll;
  pitchFiltered = cfg::COMPLEMENTARY_ALPHA * (pitchFiltered + gyroYDps * dt)
      + (1.0f - cfg::COMPLEMENTARY_ALPHA) * accelPitch;
  yawRateFiltered = 0.80f * yawRateFiltered + 0.20f * gyroZDps;

  const float roll = rollFiltered - rollZero;
  const float pitch = pitchFiltered - pitchZero;
  const int8_t rollAxis = tilt::axisFromAngle(
      roll, cfg::MAX_TILT_DEG, cfg::TILT_DEADZONE_DEG, cfg::TILT_EXPO, cfg::INVERT_ROLL);
  const int8_t pitchAxis = tilt::axisFromAngle(
      pitch, cfg::MAX_TILT_DEG, cfg::TILT_DEADZONE_DEG, cfg::TILT_EXPO, cfg::INVERT_PITCH);
  const int8_t yawAxis = tilt::axisFromRate(
      yawRateFiltered, cfg::YAW_MAX_RATE_DPS, cfg::YAW_DEADZONE_DPS, cfg::YAW_EXPO, cfg::INVERT_YAW);

  // The standard gamepad descriptor is mapped in Mode 2 by WebFPV:
  // X=yaw, Z=roll, Rz=pitch. Y is neutral; W/S supply throttle in the browser.
  gamepad.send(yawAxis, 0, rollAxis, pitchAxis, 0, 0, HAT_CENTER, 0);
  printStatus(nowMs, roll, pitch, yawRateFiltered, rollAxis, pitchAxis, yawAxis, accelRoll, accelPitch);
}
