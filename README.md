[![Build Status](https://api.travis-ci.org/OpenNMT/Tokenizer.svg?branch=master)](https://travis-ci.org/OpenNMT/Tokenizer)

# Tokenizer

Tokenizer is a C++ implementation of OpenNMT tokenization and detokenization.

## Dependencies

Compiling executables requires:

* `Boost` (`program_options`)

## Compiling

*CMake and a compiler that supports the C++11 standard are required to compile the project.*

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=<Release or Debug> ..
make
```

It will produce the dynamic library `libtokenizer.so` (or `.dylib` on Mac OS, `.dll` on Windows), and the tokenization tools `cli/tokenize` and `cli/detokenize`.

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

```
make check
```

### Adding new tests

1. Create the input raw text file `<name>_<mode>_<joiner_annotate>_<case_feature>[_<bpe_model>].raw`, where:
   * `<name>` is the name of the test case without underscore
   * `<mode>` is the value of the `--mode` option on `cli/tokenize`
   * `<joiner_annotate>` is the marker of the `--joiner_annotate` option on `cli/tokenize`
   * `<case_feature>` is the value of the `--case_feature` option on `cli/tokenize` and `cli/detokenize`
   * *(optional)* `<bpe_model>` is the name of the file in `bpe-models/` for the `--bpe_model` option on `cli/tokenize`
2. Create the expected tokenized output file `<name>.tokenized`
3. *(optional)* Create the expected tokenized output file `<name>.tokenized.new` that will be compared to the output produced with the `--joiner_new` option
3. *(optional)* Create the expected detokenized output file `<name>.detokenized`.
   If this file is not provided, the detokenization of `<name>.tokenized` and `<name>.tokenized.new` must match the raw input text.
