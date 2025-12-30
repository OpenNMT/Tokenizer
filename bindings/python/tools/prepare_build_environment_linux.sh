#!/usr/bin/env bash
set -euxo pipefail

ROOT_DIR="$PWD"
ICU_ROOT="$ROOT_DIR/icu"

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
rm -rf build
mkdir build
pushd build

cmake \
  -DLIB_ONLY=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DICU_ROOT="$ICU_ROOT" \
  ..

make -j$(nproc)
make install DESTDIR="$ROOT_DIR/build"

popd

