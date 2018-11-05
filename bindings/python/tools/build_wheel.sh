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

BOOST_VERSION=1.67.0
CMAKE_VERSION=3.12.0
PROTOBUF_VERSION=3.6.0
SENTENCEPIECE_VERSION=0.1.4

# Install protobuf.
curl -L -O https://github.com/google/protobuf/releases/download/v${PROTOBUF_VERSION}/protobuf-cpp-${PROTOBUF_VERSION}.tar.gz
tar zxfv protobuf-cpp-${PROTOBUF_VERSION}.tar.gz
cd protobuf-${PROTOBUF_VERSION}
./configure --disable-shared --with-pic
make CXXFLAGS+="-std=c++11 -O3" CFLAGS+="-std=c++11 -O3" -j4
make install || true
cd ..

# Install cmake.
curl -L -O https://cmake.org/files/v3.12/cmake-${CMAKE_VERSION}.tar.gz
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

# Fetch Boost.
BOOST_VERSION_ARCHIVE=${BOOST_VERSION//./_}
curl -fSsL -o boost_$BOOST_VERSION_ARCHIVE.tar.gz https://dl.bintray.com/boostorg/release/$BOOST_VERSION/source/boost_$BOOST_VERSION_ARCHIVE.tar.gz

# Boost default search path do not include the "m" suffix.
ln -s -f /opt/python/cp34-cp34m/include/python3.4m/ /opt/python/cp34-cp34m/include/python3.4
ln -s -f /opt/python/cp35-cp35m/include/python3.5m/ /opt/python/cp35-cp35m/include/python3.5
ln -s -f /opt/python/cp36-cp36m/include/python3.6m/ /opt/python/cp36-cp36m/include/python3.6
ln -s -f /opt/python/cp37-cp37m/include/python3.7m/ /opt/python/cp37-cp37m/include/python3.7

mkdir -p wheelhouse

for PYTHON_ROOT in /opt/python/*
do
    # Start from scratch to not deal with previously generated files.
    rm -rf boost_$BOOST_VERSION_ARCHIVE
    rm -rf Tokenizer

    # Build Boost.Python.
    tar xf boost_$BOOST_VERSION_ARCHIVE.tar.gz
    cd boost_$BOOST_VERSION_ARCHIVE
    export BOOST_ROOT=$PWD/install
    ./bootstrap.sh --prefix=$BOOST_ROOT --with-libraries=python --with-python=$PYTHON_ROOT/bin/python
    ./b2 install
    cd ..

    # Build Tokenizer.
    git clone --depth 1 --branch ${TOKENIZER_BRANCH} --single-branch $TOKENIZER_REMOTE
    cd Tokenizer
    export TOKENIZER_ROOT=$PWD/install
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=$TOKENIZER_ROOT -DCMAKE_BUILD_TYPE=Release -DLIB_ONLY=ON ..
    make -j4
    make install
    cd ..

    # Build Python wheel.
    cd bindings/python
    $PYTHON_ROOT/bin/python setup.py bdist_wheel
    export LD_LIBRARY_PATH=/usr/local/lib:$BOOST_ROOT/lib:$TOKENIZER_ROOT/lib64
    auditwheel repair dist/*.whl
    mv -f wheelhouse/*.whl $ROOT_DIR/wheelhouse

    cd $ROOT_DIR
done
