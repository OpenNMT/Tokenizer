#! /bin/bash

# Script to build Python wheels in the manylinux1 Docker environment.

set -e
set -x

ROOT_DIR=$PWD
SENTENCEPIECE_VERSION=${SENTENCEPIECE_VERSION:-0.1.8}
PYBIND11_VERSION=${PYBIND11_VERSION:-2.2.4}

# Install cmake.
/opt/python/cp37-cp37m/bin/pip install cmake
CMAKE=/opt/python/cp37-cp37m/bin/cmake

# Build SentencePiece.
curl -L -o sentencepiece-${SENTENCEPIECE_VERSION}.tar.gz -O https://github.com/google/sentencepiece/archive/v${SENTENCEPIECE_VERSION}.tar.gz
tar zxf sentencepiece-${SENTENCEPIECE_VERSION}.tar.gz
cd sentencepiece-${SENTENCEPIECE_VERSION}
mkdir build
cd build
$CMAKE ..
make
make install
cd ../..

# Build Tokenizer.
mkdir build
cd build
$CMAKE -DCMAKE_BUILD_TYPE=Release -DLIB_ONLY=ON ..
make
make install
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
    auditwheel repair $wheel
done

mv wheelhouse $ROOT_DIR
