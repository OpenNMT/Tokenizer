#! /bin/sh

# Script to build Python wheels in the manylinux1 Docker environment.

# docker pull quay.io/pypa/manylinux1_x86_64
# mkdir docker
# docker run --rm -ti -v $PWD/docker:/docker -w /docker quay.io/pypa/manylinux1_x86_64 bash
# wget --no-check-certificate https://raw.githubusercontent.com/OpenNMT/Tokenizer/master/bindings/python/tools/build_wheel.sh
# sh build_wheel.sh v1.3.0
# twine wheelhouse/*.whl

ROOT_DIR=$PWD

TOKENIZER_BRANCH=${1:-master}
TOKENIZER_REMOTE=https://github.com/OpenNMT/Tokenizer.git

# Build SentencePiece dependencies.
# See https://github.com/google/sentencepiece/blob/master/make_py_wheel.sh
wget http://ftpmirror.gnu.org/libtool/libtool-2.4.6.tar.gz
tar zxfv libtool-2.4.6.tar.gz
cd libtool-2.4.6
./configure
make -j4
make install
cd ..

git clone --depth=1 --single-branch --branch v3.5.2 https://github.com/google/protobuf.git
cd protobuf
./autogen.sh
./configure
make -j4
make install
cd ..

# Build SentencePiece.
git clone --depth=1 --single-branch https://github.com/google/sentencepiece.git
cd sentencepiece
./autogen.sh
grep -v PKG_CHECK_MODULES configure > tmp
mv tmp -f configure
chmod +x configure
LIBS+="-lprotobuf -pthread" ./configure
make -j4
make install
cd ..

# Fetch a binary release of cmake.
mkdir -p cmake && cd cmake
wget --no-check-certificate https://cmake.org/files/v3.1/cmake-3.1.0-Linux-x86_64.sh
sh cmake-3.1.0-Linux-x86_64.sh --skip-license
CMAKE=$PWD/bin/cmake
cd ..

# Fetch Boost.
wget -O boost_1_66_0.tar.gz https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz
tar xf boost_1_66_0.tar.gz

# Boost default search path do not include the "m" suffix.
ln -s -f /opt/python/cp33-cp33m/include/python3.3m/ /opt/python/cp33-cp33m/include/python3.3
ln -s -f /opt/python/cp34-cp34m/include/python3.4m/ /opt/python/cp34-cp34m/include/python3.4
ln -s -f /opt/python/cp35-cp35m/include/python3.5m/ /opt/python/cp35-cp35m/include/python3.5
ln -s -f /opt/python/cp36-cp36m/include/python3.6m/ /opt/python/cp36-cp36m/include/python3.6

mkdir -p wheelhouse

for PYTHON_ROOT in /opt/python/*
do
    # Start from scratch to not deal with previously generated files.
    rm -rf boost_1_66_0
    rm -rf Tokenizer

    # Build Boost.Python.
    tar xf boost_1_66_0.tar.gz
    cd boost_1_66_0
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
    ${CMAKE} -DCMAKE_INSTALL_PREFIX=$TOKENIZER_ROOT -DCMAKE_BUILD_TYPE=Release -DLIB_ONLY=ON ..
    make -j4
    make install
    cd ..

    # Build Python wheel.
    cd bindings/python
    $PYTHON_ROOT/bin/python -c 'import sys; exit(sys.version_info.major == 3)'
    if [ $? -eq 0 ]; then
        export BOOST_PYTHON_LIBRARY=boost_python
    else
        export BOOST_PYTHON_LIBRARY=boost_python3
    fi
    $PYTHON_ROOT/bin/python setup.py bdist_wheel
    export LD_LIBRARY_PATH=/usr/local/lib:$BOOST_ROOT/lib:$TOKENIZER_ROOT/lib
    auditwheel repair dist/*.whl
    mv -f wheelhouse/*.whl $ROOT_DIR/wheelhouse

    cd $ROOT_DIR
done
