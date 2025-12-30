#! /bin/bash

set -e
set -x

ROOT_DIR="$PWD"
ICU_ROOT="$ROOT_DIR/icu"
CMAKE_EXTRA_ARGS=""

mkdir -p "$ICU_ROOT"

# Install ICU via Homebrew
brew install icu4c
ICU_PREFIX="$(brew --prefix icu4c)"

# Copy ICU into local prefix
rsync -a "$ICU_PREFIX/" "$ICU_ROOT/"

# Remove dynamic libraries to force static linking
rm -f "$ICU_ROOT/lib/"*.dylib || true

# Explicit Apple Silicon handling
if [[ "$(uname -m)" == "arm64" ]]; then
    CMAKE_EXTRA_ARGS="-DCMAKE_OSX_ARCHITECTURES=arm64"
fi

# Install cmake
pip install "cmake==3.18.*"

# Build Tokenizer
rm -rf build
mkdir build
cd build
cmake -DLIB_ONLY=ON -DICU_ROOT="$ICU_ROOT" $CMAKE_EXTRA_ARGS ..
VERBOSE=1 make -j2 install
cd "$ROOT_DIR"

