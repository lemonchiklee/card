# ========================================
# CardCab Build & Deploy Script (PowerShell)
# ========================================
#
# Usage: .\build_and_deploy.ps1 [-QtDir "C:\Qt\5.15.2\msvc2019_64"]
#

param(
    [string]$QtDir = $env:Qt5_DIR,
    [string]$BuildType = "Release",
    [switch]$Deploy
)

$ErrorActionPreference = "Stop"

Write-Host "====================================" -ForegroundColor Cyan
Write-Host "  CardCab Build & Deploy Script" -ForegroundColor Cyan
Write-Host "====================================" -ForegroundColor Cyan
Write-Host ""

# Check Qt directory
if ([string]::IsNullOrEmpty($QtDir)) {
    Write-Host "ERROR: Qt directory not specified!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Usage examples:" -ForegroundColor Yellow
    Write-Host "  .\build_and_deploy.ps1 -QtDir 'C:\Qt\5.15.2\msvc2019_64'"
    Write-Host "  set Qt5_DIR=C:\Qt\5.15.2\msvc2019_64 && .\build_and_deploy.ps1"
    Write-Host ""
    exit 1
}

# Resolve Qt paths
$Qt5CmakeDir = Join-Path $QtDir "lib\cmake\Qt5"
$QtBinDir = Join-Path $QtDir "bin"
$WinDeployQt = Join-Path $QtBinDir "windeployqt.exe"

Write-Host "Qt Directory: $QtDir" -ForegroundColor Green
Write-Host "Qt CMake Dir: $Qt5CmakeDir" -ForegroundColor Green
Write-Host ""

# Check if Qt exists
if (!(Test-Path $Qt5CmakeDir)) {
    Write-Host "ERROR: Qt5 cmake directory not found: $Qt5CmakeDir" -ForegroundColor Red
    exit 1
}

# Create build directory
$BuildDir = Join-Path $PSScriptRoot "build"
if (!(Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Set-Location $BuildDir

# Configure with CMake
Write-Host "Configuring with CMake..." -ForegroundColor Yellow
$cmakeArgs = @(
    "-G", "Visual Studio 17 2022",
    "-A", "x64",
    "-DCMAKE_PREFIX_PATH=$Qt5CmakeDir",
    ".."
)

& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed! Trying Visual Studio 2019..." -ForegroundColor Yellow
    $cmakeArgs[1] = "Visual Studio 16 2019"
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        Write-Host "CMake configuration failed!" -ForegroundColor Red
        exit 1
    }
}

# Build
Write-Host ""
Write-Host "Building $BuildType..." -ForegroundColor Yellow
& cmake --build . --config $BuildType
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

$ExePath = Join-Path $BuildDir "$BuildType\CardCab.exe"
Write-Host ""
Write-Host "Build successful!" -ForegroundColor Green
Write-Host "Executable: $ExePath"

# Deploy Qt DLLs
if ($Deploy) {
    Write-Host ""
    Write-Host "Deploying Qt DLLs..." -ForegroundColor Yellow
    
    if (!(Test-Path $WinDeployQt)) {
        Write-Host "ERROR: windeployqt not found: $WinDeployQt" -ForegroundColor Red
        exit 1
    }
    
    # Add Qt bin to PATH temporarily
    $env:PATH = "$QtBinDir;$env:PATH"
    
    & $WinDeployQt --release --no-translations --no-system-d3d-compiler --no-opengl-sw $ExePath
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Deployment failed!" -ForegroundColor Red
        exit 1
    }
    
    Write-Host ""
    Write-Host "Deployment complete!" -ForegroundColor Green
    Write-Host "All required DLLs are now in: $(Split-Path $ExePath)"
}

Write-Host ""
Write-Host "====================================" -ForegroundColor Cyan
Write-Host "  Done!" -ForegroundColor Cyan
Write-Host "====================================" -ForegroundColor Cyan

Set-Location $PSScriptRoot
