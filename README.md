[![Build Status](https://api.travis-ci.org/OpenNMT/Tokenizer.svg?branch=master)](https://travis-ci.org/OpenNMT/Tokenizer) [![PyPI version](https://badge.fury.io/py/pyonmttok.svg)](https://badge.fury.io/py/pyonmttok)

# Tokenizer

Tokenizer is a fast, generic, and customizable text tokenization library for C++ and Python with minimal dependencies.

## Overview

By default, the Tokenizer applies a simple tokenization based on Unicode types. It can be customized in several ways:

* **Reversible tokenization**<br/>Marking joints or spaces by annotating tokens or injecting modifier characters.
* **Subword tokenization**<br/>Support for training and using BPE and SentencePiece models.
* **Advanced text segmentation**<br/>Split digits, segment on case or alphabet change, segment each character of selected alphabets, etc.
* **Case management**<br/>Lowercase text and return case information as a separate feature or inject case modifier tokens.
* **Protected sequences**<br/>Sequences can be protected against tokenization with the special characters "｟" and "｠".

See the [available options](docs/options.md) for an overview of supported features.

## Using

The Tokenizer can be used in Python, C++, or command line. Each mode exposes the same set of options.

### Python API

```bash
pip install pyonmttok
```

```python
>>> import pyonmttok
>>> tokenizer = pyonmttok.Tokenizer("conservative", joiner_annotate=True)
>>> tokens, _ = tokenizer.tokenize("Hello World!")
>>> tokens
['Hello', 'World', '￭!']
>>> tokenizer.detokenize(tokens)
'Hello World!'
```

See the [Python API description](bindings/python) for more details.

### C++ API

```cpp
#include <onmt/Tokenizer.h>

using namespace onmt;

int main() {
  Tokenizer tokenizer(Tokenizer::Mode::Conservative, Tokenizer::Flags::JoinerAnnotate);
  std::vector<std::string> tokens;
  tokenizer.tokenize("Hello World!", tokens);
}
```

See the [Tokenizer class](include/onmt/Tokenizer.h) for more details.

### Command line clients

```bash
$ echo "Hello World!" | cli/tokenize --mode conservative --joiner_annotate
Hello World ￭!
$ echo "Hello World!" | cli/tokenize --mode conservative --joiner_annotate | cli/detokenize
Hello World!
```

See the `-h` flag to list the available options.

## Development

### Dependencies

* [ICU](http://site.icu-project.org/)
* (optional) [SentencePiece](https://github.com/google/sentencepiece)
* (required by clients) [Boost](https://www.boost.org/) (`program_options`)

### Compiling

*CMake and a compiler that supports the C++11 standard are required to compile the project.*

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=<Release or Debug> ..
make
```

It will produce the dynamic library `libOpenNMTTokenizer` and tokenization clients in `cli/`.

* To compile only the library, use the `-DLIB_ONLY=ON` flag.

### Testing

The tests are using [Google Test](https://github.com/google/googletest) which is included as a Git submodule. Run the tests with:

```
test/onmt_tokenizer_test ../test/data
```
