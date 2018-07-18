[![Build Status](https://api.travis-ci.org/OpenNMT/Tokenizer.svg?branch=master)](https://travis-ci.org/OpenNMT/Tokenizer)

# Tokenizer

Tokenizer is a C++ implementation of OpenNMT tokenization and detokenization.

## Dependencies

* (optional) [SentencePiece](https://github.com/google/sentencepiece)
* (required by clients) [Boost](https://www.boost.org/) (`program_options`)

## Compiling

*CMake and a compiler that supports the C++11 standard are required to compile the project.*

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=<Release or Debug> ..
make
```

It will produce the dynamic library `libOpenNMTTokenizer.{so,dylib,dll}`, and the tokenization tools `cli/tokenize` and `cli/detokenize`.

### Options

* To compile only the library, use the `-DLIB_ONLY=ON` flag.
* To compile with the ICU unicode backend, use the `-DWITH_ICU=ON` flag.

## Using

The tokenizer can be used in several ways:

* command line clients `cli/tokenize` and `cli/detokenize`
* [C++ API](include/onmt/Tokenizer.h)
* [Python API](bindings/python)

All APIs expose the same set of options. See the [documentation](docs/options.md) for a complete description.

### Example

```bash
$ echo "Hello World!" | cli/tokenize --joiner_annotate
Hello World ￭!
```

## Testing

Tests are using the [Google Test](https://github.com/google/googletest) framework. Once installed, simply compile the project and run:

```
test/onmt_tokenizer_test ../test/data
```
