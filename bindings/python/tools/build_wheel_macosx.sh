#! /bin/bash

# Assuming local installation of protobug 3.6.0 and cmake 3.12+
# install for current version of python

set -e
set -x

ROOT_DIR=$PWD

TOKENIZER_BRANCH=${1:-master}
TOKENIZER_REMOTE=https://github.com/OpenNMT/Tokenizer.git

BOOST_VERSION=1.67.0
SENTENCEPIECE_VERSION=0.1.4

# Build SentencePiece.
curl -L -o sentencepiece-${SENTENCEPIECE_VERSION}.tar.gz -O https://github.com/google/sentencepiece/archive/v${SENTENCEPIECE_VERSION}.tar.gz
tar zxfv sentencepiece-${SENTENCEPIECE_VERSION}.tar.gz
cd sentencepiece-${SENTENCEPIECE_VERSION}
mkdir build
cd build
export SENTENCEPIECE_ROOT=$PWD/install
cmake -DCMAKE_INSTALL_PREFIX=$SENTENCEPIECE_ROOT ..
make -j4
make install
cd ../..

# Fetch Boost.
BOOST_VERSION_ARCHIVE=${BOOST_VERSION//./_}
curl -fSsL -o boost_$BOOST_VERSION_ARCHIVE.tar.gz https://dl.bintray.com/boostorg/release/$BOOST_VERSION/source/boost_$BOOST_VERSION_ARCHIVE.tar.gz

mkdir -p wheelhouse

# Only build for current python installation
PYTHON_ROOT=/usr/local

    # Start from scratch to not deal with previously generated files.
    rm -rf boost_$BOOST_VERSION_ARCHIVE
    rm -rf Tokenizer

    # Build Boost.Python.
    tar xf boost_$BOOST_VERSION_ARCHIVE.tar.gz
    cd boost_$BOOST_VERSION_ARCHIVE
    export BOOST_ROOT=$PWD/install_boost
    ./bootstrap.sh --prefix=$BOOST_ROOT --with-libraries=python --with-python=$PYTHON_ROOT/bin/python
    ./b2 install
    cd ..

    # Build Tokenizer.
    git clone --depth 1 --branch ${TOKENIZER_BRANCH} --single-branch $TOKENIZER_REMOTE
    cd Tokenizer
    export TOKENIZER_ROOT=$PWD/install
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=$TOKENIZER_ROOT -DCMAKE_BUILD_TYPE=Release -DLIB_ONLY=ON \
	  -DCMAKE_LIBRARY_PATH=$SENTENCEPIECE_ROOT/lib -DCMAKE_INCLUDE_PATH=$SENTENCEPIECE_ROOT/include \
	  ..
    make -j4
    make install
    cd ..

    install_name_tool -change "@rpath/libsentencepiece.0.dylib" ${SENTENCEPIECE_ROOT}/lib/libsentencepiece.dylib ${TOKENIZER_ROOT}/lib/libOpenNMTTokenizer.dylib
    install_name_tool -change "@rpath/libsentencepiece_train.0.dylib" ${SENTENCEPIECE_ROOT}/lib/libsentencepiece_train.dylib ${TOKENIZER_ROOT}/lib/libOpenNMTTokenizer.dylib

    # Build Python wheel first time to build tokenizer.so
    cd bindings/python
    $PYTHON_ROOT/bin/python setup.py bdist_wheel

    # Adjust @rpath to absolute path
    install_name_tool -change "@rpath/libOpenNMTTokenizer.dylib" ${TOKENIZER_ROOT}/lib/libOpenNMTTokenizer.dylib build/lib.*/pyonmttok/tokenizer.so
    install_name_tool -change "libboost_python27.dylib" ${BOOST_ROOT}/lib/libboost_python27.dylib build/lib.*/pyonmttok/tokenizer.so

    # rebuild the wheel
    $PYTHON_ROOT/bin/python setup.py bdist_wheel

    # introduce in the wheel all of the local libraries
    delocate-wheel -w fixed_wheels -v dist/*.whl

    mv -f fixed_wheels/*.whl $ROOT_DIR/wheelhouse

    cd $ROOT_DIR
