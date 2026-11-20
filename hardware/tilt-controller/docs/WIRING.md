# Wiring, neutral and direction

Follow the pin table in [README](../README.md). IO41/42 belong to the default
XH-S3E profile. XIAO GPIO8/9 are printed D9/D10, not D8/D9. AD0 low selects 0x68;
the firmware also tries 0x69. INT is not used, and the sensor is polled over I2C.

## Neutral

Hold the controller motionless in its intended rest pose. Startup calibration
averages 200 samples at 5 ms intervals. IO0/BOOT triggers a new calibration when
held for 1.2 seconds. A 3-second press followed by 2 seconds without motion makes
the hold and sample window easier to complete. Pressing RST restarts the program;
it is a different button. Holding IO0 during reset can enter the ROM bootloader.

If a level controller sends full pitch, compare `REF z=` to `REF a=` before
reflashing. A zero captured while the board was tilted can remain far from level.
A successful recenter changes the zero and keeps all three `H=` values at zero
while stationary. Do not use only a momentary centered report as acceptance.

## Directions

Mounting changes the expected signs. Test one direction at a time at low throttle.
In Settings > Check sticks, hold the axis that is wrong, then press R/Reverse.
Save mapping after the change. Leave W/S as throttle; its axis-assignment button
is deliberately disabled for this controller. If you instead change INVERT_ROLL,
INVERT_PITCH or INVERT_YAW in config.h and rebuild, re-check any browser reversal
to avoid applying the same inversion twice.

## Sensor diagnostics

The module label does not prove which chip is fitted. This assembly returned
identity 0x70, which is not treated as proof of a standard MPU6050. A missing
register read, intermittent connection or full-scale axis needs investigation
against power, wiring, current angles and the stored zero. The code does not
perform a complete chip identification or electrical continuity test.
