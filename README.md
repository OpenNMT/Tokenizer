[![Build Status](https://api.travis-ci.org/OpenNMT/Tokenizer.svg?branch=master)](https://travis-ci.org/OpenNMT/Tokenizer)

# Tokenizer

This project implements a generic and customizable text tokenization based on the original OpenNMT tokenization tools. It features:

* Fast and generic text tokenization with minimal dependencies
* Support for BPE and SentencePiece models
* Efficient training mode for learning subword models
* Customizable reversible tokenization: marking joints or spaces, with special characters or tokens
* Advanced text segmentation options: case change, alphabet change, etc.
* Protected sequences against tokenization with the special characters "｟" and "｠"
* Easy to use C++ and Python APIs

## Dependencies

* (optional) [SentencePiece](https://github.com/google/sentencepiece)
* (optional) [ICU](http://site.icu-project.org/)
* (required by clients) [Boost](https://www.boost.org/) (`program_options`)

## Compiling

*CMake and a compiler that supports the C++11 standard are required to compile the project.*

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=<Release or Debug> ..
make
```

It will produce the dynamic library `libOpenNMTTokenizer` and tokenization clients in `cli/`.

### Options

* To compile only the library, use the `-DLIB_ONLY=ON` flag.
* To compile with the ICU unicode backend, use the `-DWITH_ICU=ON` flag.

## Using

The tokenizer can be used in several ways:

* command line clients `cli/tokenize`, `cli/detokenize`, `cli/subword_learn`
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
