# SPDX-License-Identifier: GPL-3.0-or-later
param([ValidateRange(1024, 65535)][int]$Port = 4186)
$ErrorActionPreference = 'Stop'
$repo = $PSScriptRoot
$entry = Join-Path $repo 'scripts\serve.js'
$runtime = Join-Path $repo 'local-runtime'
$listeners = @(Get-NetTCPConnection -State Listen -LocalPort $Port -ErrorAction SilentlyContinue)
if ($listeners.Count) {
    $owner = Get-CimInstance Win32_Process -Filter "ProcessId=$($listeners[0].OwningProcess)"
    if ($owner.Name -eq 'node.exe' -and $owner.CommandLine.Contains($entry)) {
        Write-Output "Already running: http://127.0.0.1:$Port/?map=field"
        exit 0
    }
    throw "Port $Port is already occupied. No process was stopped."
}
New-Item -ItemType Directory -Path $runtime -Force | Out-Null
$node = (Get-Command node.exe -ErrorAction Stop).Source
$oldPort = $env:PORT
try {
    $env:PORT = [string]$Port
    $server = Start-Process -FilePath $node -ArgumentList @(('"' + $entry + '"')) -WorkingDirectory $repo -WindowStyle Hidden -RedirectStandardOutput (Join-Path $runtime 'server.log') -RedirectStandardError (Join-Path $runtime 'server-error.log') -PassThru
} finally {
    if ($null -eq $oldPort) { Remove-Item Env:PORT -ErrorAction SilentlyContinue } else { $env:PORT = $oldPort }
}
for ($attempt = 0; $attempt -lt 30; $attempt++) {
    if ($server.HasExited) { throw "Server exited. Read local-runtime/server-error.log." }
    try {
        $response = Invoke-WebRequest -UseBasicParsing -Uri "http://127.0.0.1:$Port/dist/sim.wasm" -Method Head -TimeoutSec 2
        if ($response.StatusCode -eq 200) {
            @{ pid = $server.Id; port = $Port; entry = $entry; started = (Get-Date).ToString('o') } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $runtime 'server.json') -Encoding utf8
            Write-Output "Running: http://127.0.0.1:$Port/?map=field"
            exit 0
        }
    } catch { Start-Sleep -Milliseconds 200 }
}
throw 'The server did not become ready. Read local-runtime/server-error.log.'
