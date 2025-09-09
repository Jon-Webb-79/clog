@echo off
REM ==============================================================================
REM install.bat — Configure, build, and install CLogger as a system library
REM ==============================================================================

setlocal enabledelayedexpansion

REM ---- Defaults ---------------------------------------------------------------
set PREFIX=C:\clog-install
set BUILD_TYPE=Release
set BUILD_KIND=static   REM static|shared|both
set BUILD_DIR=

REM ---- Parse args -------------------------------------------------------------
:parse_args
if "%~1"=="" goto args_done
if "%~1"=="--prefix" (
    set PREFIX=%~2
    shift
) else if "%~1"=="--shared" (
    set BUILD_KIND=shared
) else if "%~1"=="--static" (
    set BUILD_KIND=static
) else if "%~1"=="--both" (
    set BUILD_KIND=both
) else if "%~1"=="--debug" (
    set BUILD_TYPE=Debug
) else if "%~1"=="--build-dir" (
    set BUILD_DIR=%~2
    shift
) else if "%~1"=="-h" (
    echo Usage: %~nx0 [--prefix DIR] [--shared|--static|--both] [--build-dir DIR] [--debug]
    exit /b 0
) else (
    echo Unknown option: %~1
    exit /b 1
)
shift
goto parse_args
:args_done

REM ---- Paths ------------------------------------------------------------------
set SCRIPT_DIR=%~dp0
set ROOT_DIR=%SCRIPT_DIR%\..\..\clog

if not exist "%ROOT_DIR%\CMakeLists.txt" (
    echo Error: CMakeLists.txt not found at %ROOT_DIR%
    exit /b 1
)

if "%BUILD_DIR%"=="" (
    set BUILD_DIR=%ROOT_DIR%\build\install-%BUILD_KIND%-%BUILD_TYPE%
)

REM ---- Parallel jobs ----------------------------------------------------------
if defined NUMBER_OF_PROCESSORS (
    set JOBS=%NUMBER_OF_PROCESSORS%
) else (
    set JOBS=4
)

REM ---- Variant switches -------------------------------------------------------
set cmake_static=OFF
set cmake_shared=OFF
if "%BUILD_KIND%"=="static" (
    set cmake_static=ON
) else if "%BUILD_KIND%"=="shared" (
    set cmake_shared=ON
) else if "%BUILD_KIND%"=="both" (
    set cmake_static=ON
    set cmake_shared=ON
)

echo ==^> Configuring
echo     Source:     %ROOT_DIR%
echo     Build dir:  %BUILD_DIR%
echo     Build type: %BUILD_TYPE%
echo     Kind:       %BUILD_KIND% (static=%cmake_static%, shared=%cmake_shared%)
echo     Prefix:     %PREFIX%
echo.

cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" ^
  -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
  -DCMAKE_INSTALL_PREFIX="%PREFIX%" ^
  -DLOGGER_BUILD_STATIC=%cmake_static% ^
  -DLOGGER_BUILD_SHARED=%cmake_shared% ^
  -DLOGGER_BUILD_TESTS=OFF ^
  -DLOGGER_INSTALL=ON

if errorlevel 1 exit /b 1

echo ==^> Building (%JOBS% jobs)
cmake --build "%BUILD_DIR%" --parallel %JOBS% --config %BUILD_TYPE%
if errorlevel 1 exit /b 1

echo ==^> Installing to %PREFIX%
cmake --install "%BUILD_DIR%" --config %BUILD_TYPE%
if errorlevel 1 exit /b 1

echo ==^> Done.
echo     Headers: %PREFIX%\include\logger.h
if "%cmake_static%"=="ON" (
  echo     Sta

