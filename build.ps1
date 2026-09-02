# MaxCall Build Script
param(
    [string]$Configuration = "Release",
    [string]$Platform = "Win32",
    [switch]$SkipPjsip
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

Write-Host "=== MaxCall Builder ===" -ForegroundColor Cyan

$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    Write-Host "ERROR: Visual Studio with C++ tools not found!" -ForegroundColor Red
    exit 1
}
Write-Host "Visual Studio: $vsPath" -ForegroundColor Green

$msbuildPath = Get-ChildItem -Path "$vsPath\MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
$msbuild = $msbuildPath.FullName

if (-not $SkipPjsip -and -not (Test-Path "$scriptDir\..\lib\libpjproject-i386-Win32-vc14-Release-Static.lib")) {
    Write-Host "Building PJSIP..." -ForegroundColor Yellow
    $pjsipDir = "$scriptDir\..\pjsip"
    if (-not (Test-Path $pjsipDir)) {
        $pjsipUrl = "https://github.com/pjsip/pjproject/archive/refs/tags/2.14.1.tar.gz"
        Invoke-WebRequest -Uri $pjsipUrl -OutFile "$env:TEMP\pjsip.tar.gz" -UseBasicParsing
        tar -xzf "$env:TEMP\pjsip.tar.gz" -C "$scriptDir\.."
        Rename-Item "$scriptDir\..\pjproject-2.14.1" $pjsipDir
    }
    Push-Location $pjsipDir
    & cmd /c "call configure.bat --disable-video"
    $vcvars = "$vsPath\VC\Auxiliary\Build\vcvars32.bat"
    & cmd /c "call `"$vcvars`" && nmake /f Makefile.win_static"
    Pop-Location
}

Write-Host "Building MaxCall..." -ForegroundColor Yellow
$slnFile = "$scriptDir\microsip.sln"
& $msbuild $slnFile /p:Configuration=$Configuration /p:Platform=$Platform /p:OutDir=bin\$Configuration\ /verbosity:minimal /nologo

if ($LASTEXITCODE -eq 0) {
    Write-Host "BUILD SUCCESS" -ForegroundColor Green
} else {
    Write-Host "BUILD FAILED" -ForegroundColor Red
    exit 1
}
