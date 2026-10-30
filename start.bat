@echo off
title Smart Parking System Launcher
echo ==========================================================
echo    Starting Smart Parking Reservation & Management System
echo ==========================================================

REM 1. Check if backend/server.exe exists, if not build it
if not exist "%~dp0backend\server.exe" (
    echo [INFO] Building backend server.exe...
    powershell -ExecutionPolicy Bypass -File "%~dp0backend\build.ps1"
)

REM 2. Start C11 Backend Server on port 8080 in background
echo [INFO] Starting C11 Backend on http://localhost:8080...
start "Smart Parking C11 Backend" /min "%~dp0backend\server.exe" 8080

REM Wait 2 seconds for backend initialization
timeout /t 2 /nobreak >nul

REM 3. Start Next.js Frontend
echo [INFO] Starting Next.js Frontend on http://localhost:3000...
echo [INFO] Open your browser at http://localhost:3000
echo ==========================================================
cd /d "%~dp0frontend"
npm.cmd run dev
