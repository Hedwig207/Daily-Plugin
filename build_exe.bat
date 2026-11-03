@echo off
rem ============================================
rem  DailyPlugin - build Windows .exe
rem  requires: MinGW-w64 (gcc) + PowerShell
rem ============================================

where gcc >nul 2>nul
if errorlevel 1 (
    echo [ERROR] gcc not found. Please install MinGW-w64 and add it to PATH.
    exit /b 1
)

echo [1/3] Embedding templates into the executable...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\gen_templates_embed.ps1"
if errorlevel 1 (
    echo [ERROR] Template embedding failed.
    exit /b 1
)

echo [2/3] Compiling main program...
gcc -O2 -Wall -Isrc -o dp.exe src/main.c src/util.c src/template.c src/scaffold.c src/package.c src/install.c src/addon.c src/update.c src/embed.c build_gen/templates_embed.c
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo [3/3] Preparing release directory...
if not exist dist mkdir dist
copy /y dp.exe dist\dp.exe >nul

echo.
echo Done! dp.exe is self-contained (templates are embedded).
echo   dist\dp.exe
echo.
echo Usage: dp.exe help
exit /b 0