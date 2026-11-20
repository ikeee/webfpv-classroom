// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <stdint.h>

namespace tilt {

constexpr float clamp01(float v) {
  return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

constexpr float applyExpo(float unit, float expo) {
  return unit * (1.0f - expo) + unit * unit * unit * expo;
}

constexpr int8_t axisFromUnit(float unit, float expo, bool inverted) {
  return static_cast<int8_t>(inverted
      ? -static_cast<int>(applyExpo(clamp01(unit), expo) * 127.0f + 0.5f)
      : static_cast<int>(applyExpo(clamp01(unit), expo) * 127.0f + 0.5f));
}

constexpr int8_t axisFromAngle(
    float angle,
    float maxAngle,
    float deadzone,
    float expo,
    bool inverted) {
  return (maxAngle <= deadzone)
      ? 0
      : axisFromUnit(
          (angle < 0.0f ? -angle : angle) <= deadzone
              ? 0.0f
              : ((angle < 0.0f ? -angle : angle) - deadzone) / (maxAngle - deadzone),
          expo,
          (angle < 0.0f) != inverted);
}

constexpr int8_t axisFromRate(
    float rate,
    float maxRate,
    float deadzone,
    float expo,
    bool inverted) {
  return (maxRate <= deadzone)
      ? 0
      : axisFromUnit(
          (rate < 0.0f ? -rate : rate) <= deadzone
              ? 0.0f
              : ((rate < 0.0f ? -rate : rate) - deadzone) / (maxRate - deadzone),
          expo,
          (rate < 0.0f) != inverted);
}

}  // namespace tilt
