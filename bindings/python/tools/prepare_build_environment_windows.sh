#! /bin/bash

set -e
set -x

ROOT_DIR=$PWD
ICU_VERSION=${ICU_VERSION:-73.2}

curl --netrc-optional -L -O -nv https://github.com/unicode-org/icu/releases/download/release-${ICU_VERSION/./-}/icu4c-${ICU_VERSION/./_}-Win64-MSVC2019.zip
unzip *.zip
unzip *_Release/icu-windows.zip -d icu
rm -r *_Release

rm -rf build
mkdir build
cd build
cmake -DLIB_ONLY=ON -DICU_ROOT=$ROOT_DIR/icu/ -DCMAKE_INSTALL_PREFIX=$TOKENIZER_ROOT ..
cmake --build . --config Release --target install

cp $ROOT_DIR/icu/bin64/icudt*.dll $ROOT_DIR/bindings/python/pyonmttok/
cp $ROOT_DIR/icu/bin64/icuuc*.dll $ROOT_DIR/bindings/python/pyonmttok/
cp $TOKENIZER_ROOT/bin/OpenNMTTokenizer.dll $ROOT_DIR/bindings/python/pyonmttok/
