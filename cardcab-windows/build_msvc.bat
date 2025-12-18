@echo off
REM ========================================
REM CardCab Build Script for Windows (MSVC)
REM ========================================
REM
REM Prerequisites:
REM 1. Visual Studio 2019/2022 with C++ workload
REM 2. Qt5 installed (5.12+)
REM 3. PostgreSQL client libraries (libpq)
REM
REM Set Qt5_DIR environment variable to your Qt5 cmake directory, e.g.:
REM   set Qt5_DIR=C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5
REM

setlocal enabledelayedexpansion

echo ====================================
echo   CardCab Windows Build Script
echo ====================================
echo.

REM Check for Qt5_DIR
if "%Qt5_DIR%"=="" (
    echo ERROR: Qt5_DIR environment variable is not set!
    echo.
    echo Please set it to your Qt5 cmake directory, for example:
    echo   set Qt5_DIR=C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5
    echo.
    echo Common locations:
    echo   - C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5
    echo   - C:\Qt\5.12.12\msvc2017_64\lib\cmake\Qt5
    echo.
    pause
    exit /b 1
)

echo Qt5_DIR: %Qt5_DIR%
echo.

REM Create build directory
if not exist build mkdir build
cd build

echo Running CMake...
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="%Qt5_DIR%" ..

if %ERRORLEVEL% neq 0 (
    echo.
    echo CMake configuration failed!
    echo Try with Visual Studio 2019:
    echo   cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH="%Qt5_DIR%" ..
    pause
    exit /b 1
)

echo.
echo Building Release...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo.
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ====================================
echo   Build successful!
echo ====================================
echo.
echo Executable: build\Release\CardCab.exe
echo.
echo To deploy Qt DLLs, run:
echo   windeployqt build\Release\CardCab.exe
echo.

pause
