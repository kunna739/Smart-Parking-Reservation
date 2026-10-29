# C11 Backend Build Script for Windows (MinGW64 GCC)
param(
    [string]$Target = "server",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$CC = "gcc"
$CFLAGS = @("-std=c11", "-Wall", "-Wextra", "-pedantic", "-O2", "-Iinclude", "-Ilib")
$LDFLAGS = @("-lsqlite3", "-lws2_32")

$BackendDir = $PSScriptRoot
Set-Location $BackendDir

if ($Clean) {
    Write-Host "Cleaning build artifacts in $BackendDir..." -ForegroundColor Cyan
    Get-ChildItem -Path . -Filter "*.o" -Recurse | Remove-Item -Force
    Get-ChildItem -Path . -Filter "*.exe" | Remove-Item -Force
    Write-Host "Clean complete." -ForegroundColor Green
    exit 0
}

Write-Host "=== Building C11 Backend Target: $Target ===" -ForegroundColor Cyan

if ($Target -eq "test-toolchain") {
    $Src = Join-Path $BackendDir "..\tests\test_toolchain.c"
    $Out = Join-Path $BackendDir "..\tests\test_toolchain.exe"
    & $CC $CFLAGS $Src -o $Out $LDFLAGS
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Toolchain test compilation failed!"
    }
    Write-Host "Running toolchain test..." -ForegroundColor Yellow
    & $Out
    exit $LASTEXITCODE
}

# Default build for backend server
$SrcFiles = Get-ChildItem -Path "src" -Filter "*.c" | ForEach-Object { $_.FullName }
$LibFiles = Get-ChildItem -Path "lib" -Filter "*.c" -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }
$AllSrc = @()
if ($SrcFiles) { $AllSrc += $SrcFiles }
if ($LibFiles) { $AllSrc += $LibFiles }

if ($AllSrc.Count -eq 0) {
    Write-Host "No source files found in src/ yet." -ForegroundColor Yellow
    exit 0
}

$OutExe = "server.exe"
Write-Host "Compiling $($AllSrc.Count) source file(s) into $OutExe..." -ForegroundColor Cyan
& $CC $CFLAGS $AllSrc -o $OutExe $LDFLAGS

if ($LASTEXITCODE -eq 0) {
    Write-Host "[SUCCESS] $OutExe built successfully." -ForegroundColor Green
} else {
    Write-Error "[FAIL] Compilation failed with code $LASTEXITCODE"
}
