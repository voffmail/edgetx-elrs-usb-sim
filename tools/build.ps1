[CmdletBinding()]
param(
    [string]$Board = "pico2",
    [string]$BuildDir = "build",
    [switch]$Clean,
    [switch]$NoSetup
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ToolchainRoot = Join-Path $ProjectRoot ".toolchain"
$Marker = Join-Path $ToolchainRoot "ready"

if (-not (Test-Path -LiteralPath $Marker) -and -not $NoSetup) {
    & (Join-Path $PSScriptRoot "setup-toolchain.ps1")
}

$Gcc = Join-Path $ToolchainRoot "gcc\bin\arm-none-eabi-gcc.exe"
$Cmake = Join-Path $ToolchainRoot "cmake\bin\cmake.exe"
$Ninja = Join-Path $ToolchainRoot "ninja\ninja.exe"
$Python = Join-Path $ToolchainRoot "python\python.exe"
$Sdk = Join-Path $ToolchainRoot "pico-sdk"

foreach ($tool in @($Gcc, $Cmake, $Ninja, $Python)) {
    if (-not (Test-Path -LiteralPath $tool)) {
        throw "Incomplete toolchain: $tool is missing. Run tools\setup-toolchain.ps1 -Force"
    }
}

$env:PATH = "$(Split-Path $Gcc);$(Split-Path $Cmake);$(Split-Path $Ninja);$(Split-Path $Python);$env:PATH"
$env:PICO_SDK_PATH = $Sdk

$BuildPath = Join-Path $ProjectRoot $BuildDir
if ($Clean -and (Test-Path -LiteralPath $BuildPath)) {
    Remove-Item -LiteralPath $BuildPath -Recurse -Force
}

Write-Host "==> Configuring ($Board)"
& $Cmake -S $ProjectRoot -B $BuildPath -G Ninja `
    "-DPICO_BOARD=$Board" `
    "-DPICO_SDK_PATH=$Sdk" `
    "-DCMAKE_MAKE_PROGRAM=$Ninja" `
    "-DPICO_NO_PICOTOOL=1"
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed"
}

Write-Host "==> Building"
& $Cmake --build $BuildPath
if ($LASTEXITCODE -ne 0) {
    throw "Build failed"
}

$uf2 = Join-Path $BuildPath "edgetx_elrs_usb_sim.uf2"
if (-not (Test-Path -LiteralPath $uf2)) {
    throw "UF2 was not created: $uf2"
}

$target = Join-Path $ProjectRoot "edgetx_elrs_usb_sim.uf2"
Copy-Item -LiteralPath $uf2 -Destination $target -Force

Write-Host ""
Write-Host "Done: $target"
Write-Host "Flash: hold BOOT, plug USB, copy the UF2 to the RP2350 drive."
