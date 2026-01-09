#!/bin/bash
set -e
set -x

ROOT_DIR="$PWD"
ICU_ROOT="$ROOT_DIR/icu"
CMAKE_EXTRA_ARGS=""

mkdir -p "$ICU_ROOT"

# Copy ICU only if not present
if [ ! -d "$ICU_ROOT/lib" ]; then
    brew install icu4c
    ICU_PREFIX="$(brew --prefix icu4c)"
    rsync -a "$ICU_PREFIX/" "$ICU_ROOT/"
fi

# Remove dynamic libraries to force static linking
rm -f "$ICU_ROOT/lib/"*.dylib || true

if [[ "$(uname -m)" == "arm64" ]]; then
    CMAKE_EXTRA_ARGS="-DCMAKE_OSX_ARCHITECTURES=arm64"
fi

pip install cmake

# Build Tokenizer
rm -rf build
mkdir build
cd build

cmake \
  -DLIB_ONLY=ON \
  -DICU_ROOT="$ICU_ROOT" \
  -DCMAKE_INSTALL_PREFIX="$ROOT_DIR/build/install" \
  $CMAKE_EXTRA_ARGS \
  ..

VERBOSE=1 make -j2 install
cd "$ROOT_DIR"
