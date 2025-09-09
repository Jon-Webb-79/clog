@echo off
REM ===================================================================
REM debug.bat — Configure & build CLogger in Debug mode (with tests)
REM Edit SRC_DIR below to your repo path
REM ===================================================================

setlocal enabledelayedexpansion
set SRC_DIR=C:\path\to\clog\clog
set BUILD_DIR=%SRC_DIR%\build\debug

REM Parallel jobs
if defined NUMBER_OF_PROCESSORS (
  set JOBS=%NUMBER_OF_PROCESSORS%
) else (
  set JOBS=4
)

echo ==> Source: %SRC_DIR%
echo ==> Build:  %BUILD_DIR%
echo ==> Configuring (Debug, tests ON)

cmake -S "%SRC_DIR%" -B "%BUILD_DIR%" ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DLOGGER_BUILD_TESTS=ON ^
  -DLOGGER_BUILD_STATIC=ON ^
  -DLOGGER_BUILD_SHARED=OFF

if errorlevel 1 goto :error

echo ==> Building with %JOBS% jobs
cmake --build "%BUILD_DIR%" --parallel %JOBS% --config Debug
if errorlevel 1 goto :error

echo ==> Running unit tests
ctest --test-dir "%BUILD_DIR%" --output-on-failure -C Debug
if errorlevel 1 goto :error

echo Done.
exit /b 0

:error
echo Build or tests failed.
exit /b 1

