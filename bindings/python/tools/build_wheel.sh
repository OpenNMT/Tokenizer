#! /bin/bash

# Script to build Python wheels in the manylinux1 Docker environment.

set -e
set -x

ROOT_DIR=$PWD
SENTENCEPIECE_VERSION=${SENTENCEPIECE_VERSION:-0.1.8}
PYBIND11_VERSION=${PYBIND11_VERSION:-2.2.4}
ICU_VERSION=${ICU_VERSION:-64.2}
PATH=/opt/python/cp37-cp37m/bin:$PATH

# Install ICU.
curl -L -O https://github.com/unicode-org/icu/releases/download/release-${ICU_VERSION/./-}/icu4c-${ICU_VERSION/./_}-src.tgz
tar xf icu4c-*-src.tgz
cd icu/source
CFLAGS="-fPIC" CXXFLAGS="-fPIC" ./configure --disable-shared --enable-static
make -j2 install
cd $ROOT_DIR

# Install cmake.
pip install cmake

# Build SentencePiece.
curl -L -o sentencepiece-${SENTENCEPIECE_VERSION}.tar.gz -O https://github.com/google/sentencepiece/archive/v${SENTENCEPIECE_VERSION}.tar.gz
tar zxf sentencepiece-${SENTENCEPIECE_VERSION}.tar.gz
cd sentencepiece-${SENTENCEPIECE_VERSION}
mkdir build
cd build
cmake ..
make -j2 install
cd $ROOT_DIR

# Build Tokenizer.
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DLIB_ONLY=ON -DWITH_ICU=ON ..
make -j2 install
cd $ROOT_DIR

cd bindings/python
for PYTHON_ROOT in /opt/python/*
do
    $PYTHON_ROOT/bin/pip install pybind11==$PYBIND11_VERSION
    $PYTHON_ROOT/bin/python setup.py bdist_wheel
    rm -r build
done

for wheel in dist/*
do
    auditwheel show $wheel
    auditwheel repair $wheel
done

mv wheelhouse $ROOT_DIR
