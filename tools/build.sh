#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TC="$ROOT/.toolchain"
BOARD="${BOARD:-pico2}"
BUILD_DIR="${BUILD_DIR:-build}"

if [ ! -f "$TC/pico-sdk/pico_sdk_init.cmake" ]; then
    bash "$ROOT/tools/setup-toolchain.sh"
fi

export PATH="$TC/gcc/bin:$TC/cmake/bin:$TC/ninja:$PATH"
export PICO_SDK_PATH="$TC/pico-sdk"

BUILD_PATH="$ROOT/$BUILD_DIR"

cmake -S "$ROOT" -B "$BUILD_PATH" -G Ninja \
    -DPICO_BOARD="$BOARD" \
    -DPICO_SDK_PATH="$PICO_SDK_PATH" \
    -DCMAKE_MAKE_PROGRAM="$TC/ninja/ninja" \
    -DPICO_NO_PICOTOOL=1

cmake --build "$BUILD_PATH"

cp "$BUILD_PATH/edgetx_elrs_usb_sim.uf2" "$ROOT/edgetx_elrs_usb_sim.uf2"

echo
echo "Готово: $ROOT/edgetx_elrs_usb_sim.uf2"
echo "Прошивка: зажать BOOT, подключить USB, скопировать UF2 на диск RP2350."
