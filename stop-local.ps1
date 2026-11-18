# SPDX-License-Identifier: GPL-3.0-or-later
$ErrorActionPreference = 'Stop'
$record = Join-Path $PSScriptRoot 'local-runtime\server.json'
if (-not (Test-Path -LiteralPath $record)) { Write-Output 'No managed server record.'; exit 0 }
$state = Get-Content -LiteralPath $record -Raw | ConvertFrom-Json
$server = Get-CimInstance Win32_Process -Filter "ProcessId=$($state.pid)"
if ($null -eq $server) { Write-Output 'Server is already stopped.'; exit 0 }
$expected = Join-Path $PSScriptRoot 'scripts\serve.js'
if ($server.Name -ne 'node.exe' -or -not $server.CommandLine.Contains($expected)) { throw 'PID no longer belongs to this server. No process was stopped.' }
Stop-Process -Id $state.pid
Write-Output 'Local WebFPV stopped. Source and browser progress are retained.'
