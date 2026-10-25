param([switch]$NoBle)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $projectRoot
$python = 'C:\Users\Maksim\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
if (!(Test-Path -LiteralPath $python)) { $python = 'python' }
$env:CC = Join-Path $PSScriptRoot 'cc.cmd'
$env:PYTHONUTF8 = '1'
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $PSScriptRoot 'zig-cache'
$env:ZIG_LOCAL_CACHE_DIR = Join-Path $PSScriptRoot 'zig-local'
$env:X0X_FONTSET = 'terminus'
$buildArgs = @('upstream/base/tools/build.py', 'firmware/app', '--host')
if ($NoBle) { $buildArgs += '--no-ble' }
& $python @buildArgs
exit $LASTEXITCODE
