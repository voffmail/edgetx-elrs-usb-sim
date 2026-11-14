[CmdletBinding()]
param(
    [string]$SdkVersion = "2.3.1",
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ToolchainRoot = Join-Path $ProjectRoot ".toolchain"
$DownloadRoot = Join-Path $ToolchainRoot "downloads"
$Marker = Join-Path $ToolchainRoot "ready"

$GccVersion = "13.3.1-1.1"
$GccDir = Join-Path $ToolchainRoot "gcc"
$CmakeVersion = "3.31.12"
$CmakeDir = Join-Path $ToolchainRoot "cmake"
$NinjaVersion = "1.13.2"
$NinjaDir = Join-Path $ToolchainRoot "ninja"
$PythonVersion = "3.12.10"
$PythonDir = Join-Path $ToolchainRoot "python"
$SdkDir = Join-Path $ToolchainRoot "pico-sdk"

$GccUrl = "https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/download/v$GccVersion/xpack-arm-none-eabi-gcc-$GccVersion-win32-x64.zip"
$CmakeUrl = "https://github.com/Kitware/CMake/releases/download/v$CmakeVersion/cmake-$CmakeVersion-windows-x86_64.zip"
$NinjaUrl = "https://github.com/ninja-build/ninja/releases/download/v$NinjaVersion/ninja-win.zip"
$PythonUrl = "https://www.python.org/ftp/python/$PythonVersion/python-$PythonVersion-embed-amd64.zip"
$SdkUrl = "https://github.com/raspberrypi/pico-sdk.git"

function Write-Step([string]$Text) {
    Write-Host "==> $Text"
}

function Get-Download([string]$Url, [string]$Target) {
    if (Test-Path -LiteralPath $Target) {
        return
    }
    Write-Step "Downloading $(Split-Path -Leaf $Target)"
    $partial = "$Target.part"
    if (Test-Path -LiteralPath $partial) {
        Remove-Item -LiteralPath $partial -Force
    }
    $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
    if ($curl) {
        & $curl.Source -L --fail --retry 3 --output $partial $Url
        if ($LASTEXITCODE -ne 0) {
            throw "Download failed: $Url"
        }
    } else {
        Invoke-WebRequest -Uri $Url -OutFile $partial -UseBasicParsing
    }
    Move-Item -LiteralPath $partial -Destination $Target
}

function Expand-Fast([string]$Archive, [string]$Destination, [string]$Name) {
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    Write-Step "Extracting $Name"
    $tar = Get-Command tar.exe -ErrorAction SilentlyContinue
    if ($tar) {
        & $tar.Source -xf $Archive -C $Destination
        if ($LASTEXITCODE -eq 0) {
            return
        }
    }
    Expand-Archive -LiteralPath $Archive -DestinationPath $Destination -Force
}

function Reset-Dir([string]$Path) {
    if (Test-Path -LiteralPath $Path) {
        Remove-Item -LiteralPath $Path -Recurse -Force
    }
}

function Complete {
    $version = (& (Join-Path $GccDir "bin\arm-none-eabi-gcc.exe") --version | Select-Object -First 1)
    Set-Content -LiteralPath $Marker -Value @(
        "sdk=$SdkVersion"
        "gcc=$GccVersion"
        "cmake=$CmakeVersion"
        "ninja=$NinjaVersion"
        "python=$PythonVersion"
        "toolchain=$version"
    )
    Write-Host ""
    Write-Host "Toolchain ready: $ToolchainRoot"
    Write-Host "Build: powershell -ExecutionPolicy Bypass -File tools\build.ps1"
}

$complete = (Test-Path (Join-Path $GccDir "bin\arm-none-eabi-gcc.exe")) -and
            (Test-Path (Join-Path $CmakeDir "bin\cmake.exe")) -and
            (Test-Path (Join-Path $NinjaDir "ninja.exe")) -and
            (Test-Path (Join-Path $PythonDir "python.exe")) -and
            (Test-Path (Join-Path $SdkDir "pico_sdk_init.cmake"))

if ($complete -and -not $Force) {
    if (-not (Test-Path -LiteralPath $Marker)) {
        Complete
    }
    Write-Host "Toolchain already installed: $ToolchainRoot"
    exit 0
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git is required in PATH: https://git-scm.com/download/win"
}

New-Item -ItemType Directory -Force -Path $ToolchainRoot, $DownloadRoot | Out-Null
$TempRoot = Join-Path $ToolchainRoot "tmp"
Reset-Dir $TempRoot
New-Item -ItemType Directory -Force -Path $TempRoot | Out-Null

if ($Force -or -not (Test-Path (Join-Path $GccDir "bin\arm-none-eabi-gcc.exe"))) {
    $archive = Join-Path $DownloadRoot "gcc.zip"
    Get-Download $GccUrl $archive
    Reset-Dir $GccDir
    Expand-Fast $archive $TempRoot "ARM GCC"
    Move-Item -LiteralPath (Join-Path $TempRoot "xpack-arm-none-eabi-gcc-$GccVersion") -Destination $GccDir
}

if ($Force -or -not (Test-Path (Join-Path $CmakeDir "bin\cmake.exe"))) {
    $archive = Join-Path $DownloadRoot "cmake.zip"
    Get-Download $CmakeUrl $archive
    Reset-Dir $CmakeDir
    Expand-Fast $archive $TempRoot "CMake"
    Move-Item -LiteralPath (Join-Path $TempRoot "cmake-$CmakeVersion-windows-x86_64") -Destination $CmakeDir
}

if ($Force -or -not (Test-Path (Join-Path $NinjaDir "ninja.exe"))) {
    $archive = Join-Path $DownloadRoot "ninja.zip"
    Get-Download $NinjaUrl $archive
    Reset-Dir $NinjaDir
    Expand-Fast $archive $NinjaDir "Ninja"
}

if ($Force -or -not (Test-Path (Join-Path $PythonDir "python.exe"))) {
    $archive = Join-Path $DownloadRoot "python.zip"
    Get-Download $PythonUrl $archive
    Reset-Dir $PythonDir
    Expand-Fast $archive $PythonDir "Python"
}

if ($Force -or -not (Test-Path (Join-Path $SdkDir "pico_sdk_init.cmake"))) {
    Reset-Dir $SdkDir
    Write-Step "Cloning pico-sdk $SdkVersion"
    & git clone --branch $SdkVersion --depth 1 --recurse-submodules --shallow-submodules $SdkUrl $SdkDir
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to clone pico-sdk $SdkVersion"
    }
}

Reset-Dir $TempRoot
Complete
