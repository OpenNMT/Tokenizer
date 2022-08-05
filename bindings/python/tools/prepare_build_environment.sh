#! /bin/bash

set -e
set -x

ROOT_DIR=$PWD
ICU_ROOT=$ROOT_DIR/icu
CMAKE_EXTRA_ARGS=""

if [ "$CIBW_ARCHS" == "arm64" ]; then

    # Download ICU ARM64 binaries from Homebrew.
    brew fetch --force --bottle-tag=arm64_big_sur icu4c \
        | grep "Downloaded to" \
        | awk '{ print $3 }' \
        | xargs -I{} tar xf {} -C $ROOT_DIR

    mv icu4c/*.* $ICU_ROOT

    # Remove dynamic libraries to force static link.
    rm $ICU_ROOT/lib/*.dylib

    CMAKE_EXTRA_ARGS="-DCMAKE_OSX_ARCHITECTURES=arm64"

else

    # Download and compile ICU from sources.
    ICU_VERSION=${ICU_VERSION:-70.1}
    curl -L -O https://github.com/unicode-org/icu/releases/download/release-${ICU_VERSION/./-}/icu4c-${ICU_VERSION/./_}-src.tgz
    tar xf icu4c-*-src.tgz
    cd icu/source
    CFLAGS="-O3 -fPIC" CXXFLAGS="-O3 -fPIC" ./configure --disable-shared --enable-static --prefix=$ICU_ROOT
    make -j$(nproc) install

fi

cd $ROOT_DIR

# Install cmake.
pip install "cmake==3.18.*"

# Build Tokenizer.
rm -rf build
mkdir build
cd build
cmake -DLIB_ONLY=ON -DICU_ROOT=$ICU_ROOT $CMAKE_EXTRA_ARGS ..
VERBOSE=1 make -j$(nproc) install
cd $ROOT_DIR
