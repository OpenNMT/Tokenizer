#! /bin/bash

set -e
set -x

ROOT_DIR=$PWD
ICU_VERSION=${ICU_VERSION:-70.1}

# Install ICU.
if [ ! -d "$ROOT_DIR/libicu" ]; then
    curl -L -O https://github.com/unicode-org/icu/releases/download/release-${ICU_VERSION/./-}/icu4c-${ICU_VERSION/./_}-src.tgz
    tar xf icu4c-*-src.tgz
    cd icu/source
    CFLAGS="-O3 -fPIC" CXXFLAGS="-O3 -fPIC" ./configure --disable-shared --enable-static --prefix=$ROOT_DIR/libicu
    make -j2 install
    cd $ROOT_DIR
fi

# Install cmake.
pip install "cmake==3.18.*"

# Build Tokenizer.
rm -rf build
mkdir build
cd build
cmake -DLIB_ONLY=ON -DCMAKE_PREFIX_PATH=$ROOT_DIR/libicu ..
make -j2 install
cd $ROOT_DIR
