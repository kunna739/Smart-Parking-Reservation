# ====================================================================
# Smart Parking Reservation & Management System
# One-Click System Launcher (C11 Backend + Next.js Frontend)
# ====================================================================

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "   Smart Parking Reservation & Management System Launcher  " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$Root = $PSScriptRoot
$BackendExe = Join-Path $Root "backend\server.exe"

# 1. Build backend if not already built
if (-not (Test-Path $BackendExe)) {
    Write-Host "Building C11 backend binary..." -ForegroundColor Yellow
    powershell -ExecutionPolicy Bypass -File (Join-Path $Root "backend\build.ps1")
}

# 2. Launch C11 Backend Daemon on port 8080
Write-Host "Starting C11 HTTP Backend Daemon on http://localhost:8080..." -ForegroundColor Green
$backendProc = Start-Process -FilePath $BackendExe -ArgumentList "8080" -WorkingDirectory $Root -PassThru

Start-Sleep -Seconds 2

# 3. Launch Next.js Frontend Dev Server on port 3000
Write-Host "Starting Next.js Frontend on http://localhost:3000..." -ForegroundColor Green
$frontendProc = Start-Process -FilePath "powershell.exe" -ArgumentList "-NoExit -Command `"cd '$Root\frontend'; npm.cmd run dev`"" -PassThru

Write-Host ""
Write-Host "==========================================================" -ForegroundColor Green
Write-Host " System Started Successfully!" -ForegroundColor Green
Write-Host " Frontend: http://localhost:3000" -ForegroundColor Cyan
Write-Host " C11 API:  http://localhost:8080/api/facility" -ForegroundColor Cyan
Write-Host " Press Ctrl+C in this console or close windows to exit." -ForegroundColor Yellow
Write-Host "==========================================================" -ForegroundColor Green

try {
    # Keep launcher alive
    Wait-Process -Id $backendProc.Id
} finally {
    Stop-Process -Id $backendProc.Id -Force -ErrorAction SilentlyContinue
    Stop-Process -Id $frontendProc.Id -Force -ErrorAction SilentlyContinue
}
