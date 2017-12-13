## Python bindings

### Requirements

* Python
* Boost (`python`)

### API

```python
import pyonmttok

tokenizer = pyonmt.Tokenizer(
    mode: str,
    bpe_model_path="",
    joiner="￭",
    joiner_annotate=False,
    joiner_new=False,
    case_feature=False,
    segment_case=False,
    segment_numbers=False,
    segment_alphabet_change=False,
    segment_alphabet=[])

tokens, features = tokenizer.tokenize(test: str)

text = tokenizer.detokenize(tokens, features)
text = tokenizer.detokenize(tokens)  # will fail if case_feature is set.
```

### Guide

1\. Compile with Python bindings enabled:

```bash
mkdir build && cd build
cmake -DLIB_ONLY=ON -DWITH_PYTHON_BINDINGS=ON -DPYTHON_VERSION=3.5 ..
make -j4
```

2\. Extend `PYTHONPATH` to the directory containing the `pyonmttok.so` library, e.g.:

```bash
export PYTHONPATH="$PYTHONPATH:$HOME/dev/Tokenizer/build/bindings/python"
```

3\. Test the example script:

```bash
$ python3 ../bindings/python/example.py
tokenized:   ['Hello', 'World', '￭', '!']
detokenized: Hello World!
```
