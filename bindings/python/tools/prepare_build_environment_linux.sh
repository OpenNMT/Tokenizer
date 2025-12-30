#!/usr/bin/env bash
set -euxo pipefail

# Use /project as the root since that's where cibuildwheel mounts the repo
ROOT_DIR="/project"
ICU_ROOT="$ROOT_DIR/icu"
INSTALL_DIR="$ROOT_DIR/build/install"

# manylinux compiler flags (required)
export CFLAGS="-O3 -fPIC"
export CXXFLAGS="-O3 -fPIC"

# Build ICU from source
ICU_VERSION=${ICU_VERSION:-73.2}
ICU_TGZ="icu4c-${ICU_VERSION/./_}-src.tgz"
curl -LO "https://github.com/unicode-org/icu/releases/download/release-${ICU_VERSION/./-}/${ICU_TGZ}"
tar xf "$ICU_TGZ"
pushd icu/source
./configure \
  --disable-shared \
  --enable-static \
  --prefix="$ICU_ROOT"
make -j$(nproc)
make install
popd

# Build Tokenizer C++ library
rm -rf "$ROOT_DIR/build"
mkdir -p "$ROOT_DIR/build"
pushd "$ROOT_DIR/build"
cmake \
  -DLIB_ONLY=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
  -DICU_ROOT="$ICU_ROOT" \
  "$ROOT_DIR"
make -j$(nproc)
make install
popd
