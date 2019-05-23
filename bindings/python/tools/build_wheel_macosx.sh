#! /bin/bash

# WARNING: This script is OBSOLETE! Please refer to build_wheel.sh
# Assuming local installation of cmake 3.12+

set -e
set -x

ROOT_DIR=$PWD

TOKENIZER_BRANCH=${1:-master}
TOKENIZER_REMOTE=https://github.com/OpenNMT/Tokenizer.git

BOOST_VERSION=1.67.0
SENTENCEPIECE_VERSION=0.1.4
PROTOBUF_VERSION=3.6.0

# Install protobuf
curl -L -O https://github.com/google/protobuf/releases/download/v${PROTOBUF_VERSION}/protobuf-cpp-${PROTOBUF_VERSION}.tar.gz
tar zxfv protobuf-cpp-${PROTOBUF_VERSION}.tar.gz
cd protobuf-${PROTOBUF_VERSION}
./configure --disable-shared --with-pic
make CXXFLAGS+="-std=c++11 -O3 -DGOOGLE_PROTOBUF_NO_THREADLOCAL=1" \
CFLAGS+="-std=c++11 -O3 -DGOOGLE_PROTOBUF_NO_THREADLOCAL=1" -j4
make install || true
cd ..

# Build SentencePiece.
curl -L -o sentencepiece-${SENTENCEPIECE_VERSION}.tar.gz -O https://github.com/google/sentencepiece/archive/v${SENTENCEPIECE_VERSION}.tar.gz
tar zxfv sentencepiece-${SENTENCEPIECE_VERSION}.tar.gz
cd sentencepiece-${SENTENCEPIECE_VERSION}
mkdir -p build
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

curl -L -O https://bootstrap.pypa.io/get-pip.py

for URL in https://www.python.org/ftp/python/2.7.15/python-2.7.15-macosx10.9.pkg \
           https://www.python.org/ftp/python/3.6.6/python-3.6.6-macosx10.9.pkg \
           https://www.python.org/ftp/python/3.7.0/python-3.7.0-macosx10.9.pkg ; do

    VERSION=$(echo $URL | perl -pe 's/.*python\/(.*?).\d+\/python.*/$1/')
    PYTHON_INSTALL_PATH="/Library/Frameworks/Python.framework/Versions/${VERSION}/bin"
    CURRENT_PATH=${PATH}
    curl -L -o python.pkg ${URL}
    sudo installer -pkg python.pkg -target /

    if [ -f "${PYTHON_INSTALL_PATH}/python3" ]; then
      ln -sf ${PYTHON_INSTALL_PATH}/python3        ${PYTHON_INSTALL_PATH}/python
      ln -sf ${PYTHON_INSTALL_PATH}/python3-config ${PYTHON_INSTALL_PATH}/python-config
      ln -sf ${PYTHON_INSTALL_PATH}/pip3           ${PYTHON_INSTALL_PATH}/pip
      ln -sf /Library/Frameworks/Python.framework/Versions/${VERSION}/include/python${VERSION}m \
             /Library/Frameworks/Python.framework/Versions/${VERSION}/include/python${VERSION}
    fi
    
    echo "${PYTHON_INSTALL_PATH}"
    export PATH="${PYTHON_INSTALL_PATH}:${CURRENT_PATH}"
    ls -l ${PYTHON_INSTALL_PATH}
    which python
    which pip
    python --version
    sudo python get-pip.py --no-setuptools --no-wheel --ignore-installed
    pip install --upgrade setuptools
    pip install wheel
    pip install delocate


    # Start from scratch to not deal with previously generated files.
    rm -rf boost_$BOOST_VERSION_ARCHIVE
    rm -rf Tokenizer

    # Build Boost.Python.
    tar xf boost_$BOOST_VERSION_ARCHIVE.tar.gz
    cd boost_$BOOST_VERSION_ARCHIVE
    export BOOST_ROOT=$PWD/install_boost
    ./bootstrap.sh --prefix=$BOOST_ROOT --with-libraries=python --with-python=$PYTHON_INSTALL_PATH/python
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
    python setup.py bdist_wheel

    # Adjust @rpath to absolute path
    install_name_tool -change "@rpath/libOpenNMTTokenizer.dylib" ${TOKENIZER_ROOT}/lib/libOpenNMTTokenizer.dylib build/lib.*/pyonmttok/tokenizer*.so
    LIBBOOST_PYTHON=$(otool -L build/lib.*/pyonmttok/tokenizer*.so | grep boost_python | perl -pe 's/.*(libboost.*\.dylib).*/$1/')
    install_name_tool -change ${LIBBOOST_PYTHON} ${BOOST_ROOT}/lib/${LIBBOOST_PYTHON} build/lib.*/pyonmttok/tokenizer*.so

    # rebuild the wheel
    python setup.py bdist_wheel

    # introduce in the wheel all of the local libraries
    delocate-wheel -w fixed_wheels -v dist/*.whl

    mv -f fixed_wheels/*.whl $ROOT_DIR/wheelhouse

    cd $ROOT_DIR

done
