#! /bin/bash

# Script to build Python wheels in the manylinux1 Docker environment.

# docker pull quay.io/pypa/manylinux1_x86_64
# mkdir docker
# docker run --rm -ti -v $PWD/docker:/docker -w /docker quay.io/pypa/manylinux1_x86_64 bash
# curl -L -O https://raw.githubusercontent.com/OpenNMT/Tokenizer/master/bindings/python/tools/build_wheel.sh
# bash build_wheel.sh v1.3.0
# twine upload wheelhouse/*.whl

set -e
set -x

ROOT_DIR=$PWD

TOKENIZER_BRANCH=${1:-master}
TOKENIZER_REMOTE=https://github.com/OpenNMT/Tokenizer.git

PYBIND11_VERSION=2.2.4
CMAKE_VERSION=3.13.5
SENTENCEPIECE_VERSION=0.1.8

# Install cmake.
curl -L -O https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}.tar.gz
tar zxfv cmake-${CMAKE_VERSION}.tar.gz
cd cmake-${CMAKE_VERSION}
./bootstrap
make -j4
make install
cd ..

# Build SentencePiece.
curl -L -o sentencepiece-${SENTENCEPIECE_VERSION}.tar.gz -O https://github.com/google/sentencepiece/archive/v${SENTENCEPIECE_VERSION}.tar.gz
tar zxfv sentencepiece-${SENTENCEPIECE_VERSION}.tar.gz
cd sentencepiece-${SENTENCEPIECE_VERSION}
mkdir build
cd build
cmake ..
make -j4
make install
cd ../..

# Download pybind11.
curl -L -o pybind11-${PYBIND11_VERSION}.tar.gz -O https://github.com/pybind/pybind11/archive/v${PYBIND11_VERSION}.tar.gz
tar zxfv pybind11-${PYBIND11_VERSION}.tar.gz
export PYBIND11_ROOT=$PWD/pybind11-${PYBIND11_VERSION}

# Build Tokenizer.
git clone --depth 1 --branch ${TOKENIZER_BRANCH} --single-branch ${TOKENIZER_REMOTE}
cd Tokenizer
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DLIB_ONLY=ON ..
make -j4
make install
cd ..

cd bindings/python
for PYTHON_ROOT in /opt/python/*
do
    $PYTHON_ROOT/bin/python setup.py bdist_wheel
done

for wheel in dist/*
do
    auditwheel repair $wheel
done

mv wheelhouse $ROOT_DIR
