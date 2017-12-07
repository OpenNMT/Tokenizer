[![Build Status](https://api.travis-ci.org/OpenNMT/Tokenizer.svg?branch=master)](https://travis-ci.org/OpenNMT/Tokenizer)

# Tokenizer

Tokenizer is a C++ implementation of OpenNMT tokenization and detokenization.

## Dependencies

Compiling executables requires:

* `Boost` (`program_options`)

Compiling tests requires:

* [Google Test](https://github.com/google/googletest)

## Compiling

*CMake and a compiler that supports the C++11 standard are required to compile the project.*

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=<Release or Debug> ..
make
```

It will produce the dynamic library `libOpenNMTTokenizer.so` (or `.dylib` on Mac OS, `.dll` on Windows), and the tokenization tools `cli/tokenize` and `cli/detokenize`.

### Options

* To compile only the library, use the `-DLIB_ONLY=ON` flag.

## Using

### Clients

See `--help` on the clients to discover available options and usage. They have the same interface as their Lua counterpart.

### Library

This project is also a convenient way to apply OpenNMT tokenization in existing software.

See:

* `include/onmt/Tokenizer.h` to apply OpenNMT's tokenization and detokenization

## Testing

Tests are using the [Google Test](https://github.com/google/googletest) framework. Once installed, simply compile the project and run:

```
test/onmt_tokenizer_test ../test/data
```
