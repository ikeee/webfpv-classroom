// SPDX-License-Identifier: GPL-3.0-or-later
#include "axis_map.h"

static_assert(tilt::axisFromAngle(0.0f, 45.0f, 3.0f, 0.0f, false) == 0,
              "level must read centre");
static_assert(tilt::axisFromAngle(2.0f, 45.0f, 3.0f, 0.0f, false) == 0,
              "small noise must stay in the deadzone");
static_assert(tilt::axisFromAngle(45.0f, 45.0f, 3.0f, 0.0f, false) == 127,
              "full angle must reach full positive travel");
static_assert(tilt::axisFromAngle(-45.0f, 45.0f, 3.0f, 0.0f, false) == -127,
              "full opposite angle must reach full negative travel");
static_assert(tilt::axisFromAngle(90.0f, 45.0f, 3.0f, 0.0f, false) == 127,
              "travel past the end must clamp");
static_assert(tilt::axisFromAngle(22.5f, 45.0f, 3.0f, 0.0f, true) == -59,
              "inversion must flip the sign");
static_assert(tilt::axisFromRate(0.0f, 180.0f, 8.0f, 0.0f, false) == 0,
              "zero yaw rate must read centre");
static_assert(tilt::axisFromRate(8.0f, 180.0f, 8.0f, 0.0f, false) == 0,
              "small yaw rate must stay in the deadzone");
static_assert(tilt::axisFromRate(180.0f, 180.0f, 8.0f, 0.0f, false) == 127,
              "full yaw rate must reach full travel");
static_assert(tilt::axisFromAngle(22.5f, 45.0f, 3.0f, 0.55f, false)
                  < tilt::axisFromAngle(22.5f, 45.0f, 3.0f, 0.0f, false),
              "expo must soften the centre");
static_assert(tilt::axisFromAngle(45.0f, 45.0f, 3.0f, 0.55f, false) == 127,
              "expo must still reach the endpoint");

void setup() {}
void loop() {}
