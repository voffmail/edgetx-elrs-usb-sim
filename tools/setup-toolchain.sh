#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TC="$ROOT/.toolchain"
SDK_VERSION="${SDK_VERSION:-2.3.1}"

GCC_VERSION="13.3.1-1.1"
CMAKE_VERSION="3.31.12"
NINJA_VERSION="1.13.2"

GCC_DIR="$TC/gcc"
CMAKE_DIR="$TC/cmake"
NINJA_DIR="$TC/ninja"
SDK_DIR="$TC/pico-sdk"

step() {
    echo "==> $1"
}

fetch() {
    local url="$1"
    local target="$2"
    if [ -f "$target" ]; then
        return
    fi
    step "Скачивание $(basename "$target")"
    curl -L --fail --retry 3 -o "$target" "$url"
}

case "$(uname -s)-$(uname -m)" in
    Linux-x86_64)
        GCC_URL="https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/download/v${GCC_VERSION}/xpack-arm-none-eabi-gcc-${GCC_VERSION}-linux-x64.tar.gz"
        CMAKE_URL="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz"
        NINJA_URL="https://github.com/ninja-build/ninja/releases/download/v${NINJA_VERSION}/ninja-linux.zip"
        ;;
    *)
        echo "Для этой ОС скрипт не поддерживается."
        echo "Установи системные пакеты: cmake, ninja, git, python3, arm-none-eabi-gcc."
        echo "Например: sudo apt install cmake ninja-build git python3 gcc-arm-none-eabi libnewlib-arm-none-eabi"
        exit 1
        ;;
esac

for tool in git curl python3; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Не найден $tool в PATH"
        exit 1
    fi
done

mkdir -p "$TC/downloads"

if [ ! -x "$GCC_DIR/bin/arm-none-eabi-gcc" ]; then
    fetch "$GCC_URL" "$TC/downloads/gcc.tar.gz"
    step "Распаковка ARM GCC"
    rm -rf "$TC/tmp"
    mkdir -p "$TC/tmp"
    tar -xzf "$TC/downloads/gcc.tar.gz" -C "$TC/tmp"
    rm -rf "$GCC_DIR"
    mv "$TC/tmp/xpack-arm-none-eabi-gcc-${GCC_VERSION}" "$GCC_DIR"
fi

if [ ! -x "$CMAKE_DIR/bin/cmake" ]; then
    fetch "$CMAKE_URL" "$TC/downloads/cmake.tar.gz"
    step "Распаковка CMake"
    tar -xzf "$TC/downloads/cmake.tar.gz" -C "$TC"
    rm -rf "$CMAKE_DIR"
    mv "$TC/cmake-${CMAKE_VERSION}-linux-x86_64" "$CMAKE_DIR"
fi

if [ ! -x "$NINJA_DIR/ninja" ]; then
    fetch "$NINJA_URL" "$TC/downloads/ninja.zip"
    step "Распаковка Ninja"
    mkdir -p "$NINJA_DIR"
    python3 - "$TC/downloads/ninja.zip" "$NINJA_DIR" <<'PY'
import sys, zipfile
with zipfile.ZipFile(sys.argv[1]) as archive:
    archive.extractall(sys.argv[2])
PY
    chmod +x "$NINJA_DIR/ninja"
fi

if [ ! -f "$SDK_DIR/pico_sdk_init.cmake" ]; then
    step "Клонирование pico-sdk $SDK_VERSION"
    rm -rf "$SDK_DIR"
    git clone --branch "$SDK_VERSION" --depth 1 --recurse-submodules --shallow-submodules \
        https://github.com/raspberrypi/pico-sdk.git "$SDK_DIR"
fi

rm -rf "$TC/tmp"
echo "Тулчейн готов: $TC"
echo "Сборка: bash tools/build.sh"
