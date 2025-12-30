#!/usr/bin/env bash
set -euo pipefail
set -x

ROOT_DIR="$PWD"
INSTALL_PREFIX="$ROOT_DIR/build/install"
ICU_ROOT="$INSTALL_PREFIX"

mkdir -p "$INSTALL_PREFIX"

# -----------------------------
# Build ICU from source (static)
# -----------------------------
ICU_VERSION=${ICU_VERSION:-73.2}

curl -LO https://github.com/unicode-org/icu/releases/download/release-${ICU_VERSION/./-}/icu4c-${ICU_VERSION/./_}-src.tgz
tar xf icu4c-*-src.tgz

pushd icu/source

CFLAGS="-O3 -fPIC" \
CXXFLAGS="-O3 -fPIC" \
./configure \
  --disable-shared \
  --enable-static \
  --prefix="$ICU_ROOT"

make -j$(nproc)
make install

popd

# -----------------------------
# Build Tokenizer (C++)
# -----------------------------
pip install "cmake<4"

rm -rf build
mkdir build
cd build

cmake \
  -DLIB_ONLY=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
  -DICU_ROOT="$ICU_ROOT" \
  ..

make -j$(nproc)
make install

cd "$ROOT_DIR"

echo "Linux build complete"

