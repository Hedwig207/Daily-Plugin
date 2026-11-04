@echo off
rem ============================================================
rem  Build the DailyPlugin "mc" addon package (.dplug)
rem
rem  - copies the 4 loader templates (minus local build caches)
rem    into packs\mc\templates
rem  - packs packs\mc into dist\mc.dplug via "dp addon pack"
rem
rem  Usage:  make_mc_addon.bat
rem ============================================================
setlocal enabledelayedexpansion

set "SRC=templates"
set "DST=packs\mc\templates"
set "DPLUG=dist\mc.dplug"

echo == Staging templates into %DST% ==
if exist "%DST%" rmdir /s /q "%DST%"
mkdir "%DST%" 2>nul

set "LOADERS=forge fabric neoforge quilt"
for %%L in (%LOADERS%) do (
    if exist "%SRC%\%%L" (
        echo   copy %%L
        xcopy "%SRC%\%%L" "%DST%\%%L" /e /i /q /y >nul
        if exist "%DST%\%%L\.gradle" rmdir /s /q "%DST%\%%L\.gradle"
        if exist "%DST%\%%L\build"    rmdir /s /q "%DST%\%%L\build"
    ) else (
        echo   [skip] %%L not found
    )
)

echo.
echo == Packing into %DPLUG% ==
if exist "dist" (
    rem ok
) else (
    mkdir dist
)
if exist "%DPLUG%" del /q "%DPLUG%"
dp.exe addon pack packs\mc --output %DPLUG%
if errorlevel 1 (
    echo [ERROR] packing failed.
    exit /b 1
)

echo.
echo Done: %DPLUG%
echo Install with:  dp addon install %DPLUG%
endlocal