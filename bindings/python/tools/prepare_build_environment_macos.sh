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

export DYLD_LIBRARY_PATH="$ICU_ROOT/lib:$DYLD_LIBRARY_PATH"

if [[ "$(uname -m)" == "arm64" ]]; then
    CMAKE_EXTRA_ARGS="-DCMAKE_OSX_ARCHITECTURES=arm64"
fi

pip install cmake

rm -rf "$ROOT_DIR/build"
mkdir -p "$ROOT_DIR/build"

cmake \
  -S "$ROOT_DIR" \
  -B "$ROOT_DIR/build" \
  -DLIB_ONLY=ON \
  -DBUILD_SHARED_LIBS=OFF \
  -DICU_ROOT="$ICU_ROOT" \
  -DCMAKE_INSTALL_PREFIX="$ROOT_DIR/build/install" \
  -DCMAKE_MACOSX_RPATH=ON \
  -DCMAKE_INSTALL_RPATH="$ICU_ROOT/lib" \
  $CMAKE_EXTRA_ARGS

cmake --build "$ROOT_DIR/build" --target install -j2

