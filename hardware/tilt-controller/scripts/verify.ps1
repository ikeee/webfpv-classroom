# SPDX-License-Identifier: GPL-3.0-or-later
param(
    [string]$ArduinoCli = '',
    [ValidateSet('xh-s3e', 'xiao')][string]$Board = 'xh-s3e',
    [switch]$SkipWebFPV
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$web = [IO.Path]::GetFullPath((Join-Path $root '..\..'))
$build = Join-Path $root ".build\$Board"
New-Item -ItemType Directory -Force -Path $build | Out-Null
if (-not $ArduinoCli) {
    $found = Get-Command arduino-cli -ErrorAction SilentlyContinue
    if ($found) { $ArduinoCli = $found.Source }
    else { $ArduinoCli = Join-Path $env:ProgramFiles 'Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe' }
}
if (-not (Test-Path -LiteralPath $ArduinoCli)) {
    throw 'Arduino CLI not found. Put arduino-cli on PATH or pass -ArduinoCli.'
}
$fqbn = 'esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB'
$profileArgs = @()
if ($Board -eq 'xiao') {
    $fqbn = 'esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=cdc,FlashSize=8M'
    $profileArgs = @('--build-property', 'compiler.cpp.extra_flags=-DWEBFPV_BOARD_XIAO')
}
# Stage the test beside an exact copy of the production header. No machine paths
# are embedded in the sketch and no generated copies are committed.
$test = Join-Path $build 'test-source\axis_map_test'
New-Item -ItemType Directory -Force -Path $test | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'tests\axis_map_test\axis_map_test.ino') -Destination $test
Copy-Item -LiteralPath (Join-Path $root 'firmware\webfpv_tilt_pad\axis_map.h') -Destination $test
Write-Host "Compiling 11 axis-map assertions for $Board..."
& $ArduinoCli compile --fqbn $fqbn --build-path (Join-Path $build 'axis-map-test') @profileArgs $test 2>&1 |
    Tee-Object -FilePath (Join-Path $build 'axis-map-test.log')
if ($LASTEXITCODE -ne 0) { throw 'Axis-map assertions failed' }
Write-Host "Compiling $Board tilt firmware..."
& $ArduinoCli compile --fqbn $fqbn --warnings all --build-path (Join-Path $build 'firmware') @profileArgs (Join-Path $root 'firmware\webfpv_tilt_pad') 2>&1 |
    Tee-Object -FilePath (Join-Path $build 'firmware-compile.log')
if ($LASTEXITCODE -ne 0) { throw 'Firmware compile failed' }
if (-not $SkipWebFPV) {
    Push-Location $web
    try {
        & node scripts/input-selftest.js
        if ($LASTEXITCODE -ne 0) { throw 'WebFPV input checks failed' }
    } finally { Pop-Location }
}
Write-Host 'Verification complete. No serial port was opened and nothing was flashed.'
