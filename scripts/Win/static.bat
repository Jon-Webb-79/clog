@echo off
REM ==============================================================================
REM static.bat — Configure & build CLogger as a static library
REM ==============================================================================

setlocal enabledelayedexpansion

REM ---- Defaults ---------------------------------------------------------------
set BUILD_TYPE=Release
set PREFIX=C:\clog-install
set DO_INSTALL=no

REM ---- Parse args -------------------------------------------------------------
:parse_args
if "%~1"=="" goto args_done
if "%~1"=="--debug" (
    set BUILD_TYPE=Debug
) else if "%~1"=="--prefix" (
    set PREFIX=%~2
    shift
) else if "%~1"=="--install" (
    set DO_INSTALL=yes
) else if "%~1"=="--no-install" (
    set DO_INSTALL=no
) else if "%~1"=="-h" (
    echo Usage: %~nx0 [--debug] [--prefix DIR] [--install^|--no-install]
    exit /b 0
) else (
    echo Unknown option: %~1
    exit /b 1
)
shift
goto parse_args
:args_done

REM ---- Paths & environment ----------------------------------------------------
set SCRIPT_DIR=%~dp0
set ROOT_DIR=%SCRIPT_DIR%\..\..\clog

if not exist "%ROOT_DIR%\CMakeLists.txt" (
    echo Error: CMakeLists.txt not found at %ROOT_DIR%
    exit /b 1
)

set BUILD_DIR=%ROOT_DIR%\build\static-%BUILD_TYPE%

REM Detect CPU count
if defined NUMBER_OF_PROCESSORS (
    set JOBS=%NUMBER_OF_PROCESSORS%
) else (
    set JOBS=4
)

echo ==^> Configuring static library
echo     Source dir:  %ROOT_DIR%
echo     Build dir:   %BUILD_DIR%
echo     Build type:  %BUILD_TYPE%
echo     Install to:  %PREFIX%
echo     Install now: %DO_INSTALL%

cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" ^
  -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
  -DLOGGER_BUILD_STATIC=ON ^
  -DLOGGER_BUILD_SHARED=OFF ^
  -DLOGGER_BUILD_TESTS=OFF ^
  -DLOGGER_INSTALL=ON ^
  -DCMAKE_INSTALL_PREFIX="%PREFIX%"

if errorlevel 1 exit /b 1

echo ==^> Building with %JOBS% jobs
cmake --build "%BUILD_DIR%" --parallel %JOBS% --config %BUILD_TYPE%
if errorlevel 1 exit /b 1

if "%DO_INSTALL%"=="yes" (
  echo ==^> Installing to %PREFIX%
  cmake --install "%BUILD_DIR%" --config %BUILD_TYPE%
  if errorlevel 1 exit /b 1
)

echo ==^> Done.
echo Static library should be at: %BUILD_DIR%\logger.lib (and in %PREFIX%\lib if installed)

endlocal

