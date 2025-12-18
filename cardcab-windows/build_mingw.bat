@echo off
REM ========================================
REM CardCab Build Script for Windows (MinGW)
REM ========================================
REM
REM Prerequisites:
REM 1. MinGW-w64 (MSYS2 recommended)
REM 2. Qt5 installed for MinGW
REM 3. PostgreSQL client libraries (libpq)
REM
REM Set Qt5_DIR environment variable to your Qt5 cmake directory, e.g.:
REM   set Qt5_DIR=C:\Qt\5.15.2\mingw81_64\lib\cmake\Qt5
REM

setlocal enabledelayedexpansion

echo ====================================
echo   CardCab Windows Build (MinGW)
echo ====================================
echo.

REM Check for Qt5_DIR
if "%Qt5_DIR%"=="" (
    echo ERROR: Qt5_DIR environment variable is not set!
    echo.
    echo Please set it to your Qt5 cmake directory, for example:
    echo   set Qt5_DIR=C:\Qt\5.15.2\mingw81_64\lib\cmake\Qt5
    echo.
    pause
    exit /b 1
)

echo Qt5_DIR: %Qt5_DIR%
echo.

REM Create build directory
if not exist build mkdir build
cd build

echo Running CMake with MinGW Makefiles...
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%Qt5_DIR%" ..

if %ERRORLEVEL% neq 0 (
    echo.
    echo CMake configuration failed!
    echo Make sure MinGW is in your PATH
    pause
    exit /b 1
)

echo.
echo Building...
mingw32-make -j%NUMBER_OF_PROCESSORS%

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
echo Executable: build\CardCab.exe
echo.
echo To deploy Qt DLLs, run:
echo   windeployqt build\CardCab.exe
echo.

pause
