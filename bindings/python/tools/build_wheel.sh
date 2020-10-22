#! /bin/bash

# Script to build Python wheels in the manylinux1 Docker environment.

set -e
set -x

ROOT_DIR=$PWD
PYBIND11_VERSION=${PYBIND11_VERSION:-2.6.0}
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
pip install "cmake==3.13.*"

# Build Tokenizer.
mkdir build
cd build
cmake -DLIB_ONLY=ON ..
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
