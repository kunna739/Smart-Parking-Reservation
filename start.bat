@echo off
title Smart Parking System Launcher
echo ==========================================================
echo    Starting Smart Parking Reservation & Management System
echo ==========================================================

cd /d "%~dp0"
node scripts\dev.js
if %ERRORLEVEL% neq 0 (
    echo.
    echo Launcher encountered an error.
    pause
)
