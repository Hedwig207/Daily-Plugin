@echo off
rem ============================================================
rem  DailyPlugin - quick push script (Windows)
rem  Usage:  push.bat [commit message]
rem  - rebuilds dp.exe (so templates/style changes are included)
rem  - inits git if needed, sets the remote to the GitHub repo,
rem    commits everything, and pushes to main.
rem  Repo:  https://github.com/Hedwig207/Daily-Plugin
rem ============================================================
setlocal enabledelayedexpansion

set "REPO_URL=https://github.com/Hedwig207/Daily-Plugin.git"
set "BRANCH=main"

rem ---- commit message ----
set "MSG=%~1"
if "%MSG%"=="" set "MSG=Update DailyPlugin (auto push)"

echo ============================================
echo  [1/5] Building dp.exe ...
echo ============================================
call build_exe.bat
if errorlevel 1 (
    echo [ERROR] Build failed. Aborting push.
    exit /b 1
)

echo.
echo ============================================
echo  [2/5] Checking git repository ...
echo ============================================
git rev-parse --git-dir >nul 2>&1
if errorlevel 1 (
    echo Initialising new git repository...
    git init
)
rem Always ensure we are on the target branch (rename if needed).
git branch -M "%BRANCH%"

echo.
echo ============================================
echo  [3/5] Setting remote ...
echo ============================================
git remote remove origin >nul 2>&1
git remote add origin "%REPO_URL%"

echo.
echo ============================================
echo  [4/5] Staging and committing ...
echo ============================================
git add -A
if errorlevel 1 (
    echo [ERROR] git add failed.
    exit /b 1
)
git diff --cached --quiet
if not errorlevel 1 (
    echo Nothing to commit - working tree clean.
    goto :check_branch

)
git commit -m "%MSG%"
if errorlevel 1 (
    echo [ERROR] Commit failed.
    exit /b 1
)

:check_branch
echo.
echo ============================================
echo  [5/5] Pushing to "%BRANCH%" ...
echo ============================================
git pull --rebase origin "%BRANCH%" >nul 2>&1
git push -u origin "%BRANCH%"
if errorlevel 1 (
    echo [ERROR] Push failed. Check credentials / network.
    exit /b 1
)

echo.
echo Done! Pushed to %REPO_URL% (%BRANCH%)
endlocal