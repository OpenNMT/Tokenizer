# Python API

```bash
pip install pyonmttok
```

**Table of contents**

1. [Tokenization](#tokenization)
1. [Subword learning](#subword-learning)
1. [Token API](#token-api)
1. [Utilities](#utilities)

## Tokenization

### Example

```python
>>> import pyonmtok
>>> tokenizer = pyonmttok.Tokenizer("aggressive", joiner_annotate=True)
>>> tokens, _ = tokenizer.tokenize("Hello World!")
>>> tokens
['Hello', 'World', '￭!']
```

### Interface

#### Constructor

```python
tokenizer = pyonmttok.Tokenizer(
    mode: str,
    *,
    bpe_model_path: str = "",
    bpe_dropout: float = 0,
    vocabulary_path: str = "",
    vocabulary_threshold: int = 0,
    sp_model_path: str = "",
    sp_nbest_size: int = 0,
    sp_alpha: float = 0.1,
    joiner: str = "￭",
    joiner_annotate: bool = False,
    joiner_new: bool = False,
    support_prior_joiners: bool = False,
    spacer_annotate: bool = False,
    spacer_new: bool = False,
    case_feature: bool = False,
    case_markup: bool = False,
    no_substitution: bool = False,
    preserve_placeholders: bool = False,
    preserve_segmented_tokens: bool = False,
    segment_case: bool = False,
    segment_numbers: bool = False,
    segment_alphabet_change: bool = False,
    segment_alphabet: Optional[List[str]] = None)

# Copy constructor.
tokenizer = pyonmttok.Tokenizer(tokenizer: pyonmttok.Tokenizer)
```

See the [documentation](../../docs/options.md) for a description of each tokenization option.

#### Tokenization

```python
# By default, tokenize returns the tokens and features.
tokenizer.tokenize(text: str) -> Tuple[List[str], List[List[str]]]

# The as_token_objects flag can alternatively return Token objects (see below).
tokenizer.tokenize(text: str, as_token_objects=True) -> List[pyonmttok.Token]

# Tokenize a file.
tokenizer.tokenize_file(input_path: str, output_path: str, num_threads: int = 1)
```

#### Detokenization

```python
# The detokenize method converts tokens back to a string.
tokenizer.detokenize(
    tokens: Union[List[str], List[pyonmttok.Token]],
    features: Optional[List[List[str]]] = None
) -> str

# The detokenize_with_ranges method also returns a dictionary mapping a token
# index to a range in the detokenized text.
# Set merge_ranges=True to merge consecutive ranges, e.g. subwords of the same
# token in case of subword tokenization.
# Set unicode_ranges=True to return ranges over Unicode characters instead of bytes.
tokenizer.detokenize_with_ranges(
    tokens: Union[List[str], List[pyonmttok.Token]],
    merge_ranges: bool = True,
    unicode_ranges: bool = True
) -> Tuple[str, Dict[int, Pair[int, int]]]

# Detokenize a file.
tokenizer.detokenize_file(input_path: str, output_path: str)
```

## Subword learning

### Example

The Python wrapper supports BPE and SentencePiece subword learning through a common interface:

**1\. Create the subword learner with the tokenization you want to apply, e.g.:**

```python
# BPE is trained and applied on the tokenization output before joiner (or spacer) annotations.
tokenizer = pyonmttok.Tokenizer("aggressive", joiner_annotate=True, segment_numbers=True)
learner = pyonmttok.BPELearner(tokenizer=tokenizer, symbols=32000)

# SentencePiece can learn from raw sentences so a tokenizer in not required.
learner = pyonmttok.SentencePieceLearner(vocab_size=32000, character_coverage=0.98)
```

**2\. Feed some raw data:**

```python
# Feed detokenized sentences:
learner.ingest("Hello world!")
learner.ingest("How are you?")

# or detokenized text files:
learner.ingest_file("/data/train1.en")
learner.ingest_file("/data/train2.en")
```

**3\. Start the learning process:**

```python
tokenizer = learner.learn("/data/model-32k")
```

The returned `tokenizer` instance can be used to apply subword tokenization on new data.

### Interface

```python
# See https://github.com/rsennrich/subword-nmt/blob/master/subword_nmt/learn_bpe.py
# for argument documentation.
learner = pyonmttok.BPELearner(
    tokenizer: Optional[pyonmttok.Tokenizer] = None,  # Defaults to tokenization mode "space".
    symbols: int = 10000,
    min_frequency: int = 2,
    total_symbols: bool = False)

# See https://github.com/google/sentencepiece/blob/master/src/spm_train_main.cc
# for available training options.
learner = pyonmttok.SentencePieceLearner(
    tokenizer: Optional[pyonmttok.Tokenizer] = None,  # Defaults to tokenization mode "none".
    keep_vocab: bool = False,  # Keep the generated vocabulary (model_path will act like model_prefix in spm_train)
    **training_options)

learner.ingest(text: str)
learner.ingest_file(path: str)
learner.ingest_token(token: Union[str, pyonmttok.Token])

learner.learn(model_path: str, verbose: bool = False) -> pyonmttok.Tokenizer
```

## Token API

The Token API allows to tokenize text into `pyonmttok.Token` objects. This API can be useful to apply some logics at the token level but still retain enough information to write the tokenization on disk or detokenize.

### Example

```python
>>> tokenizer = pyonmttok.Tokenizer("aggressive", joiner_annotate=True)
>>> tokens = tokenizer.tokenize("Hello World!", as_token_objects=True)
>>> tokens
[Token('Hello'), Token('World'), Token('!', join_left=True)]
>>> tokens[-1].surface
'!'
>>> tokenizer.serialize_tokens(tokens)[0]
['Hello', 'World', '￭!']
>>> tokens[-1].surface = '.'
>>> tokenizer.serialize_tokens(tokens)[0]
['Hello', 'World', '￭.']
>>> tokenizer.detokenize(tokens)
'Hello World.'
```

### Interface

The `pyonmttok.Token` class has the following attributes:

* `surface`: a string, the token value
* `type`: a `pyonmttok.TokenType` value, the type of the token
* `join_left`: a boolean, whether the token should be joined to the token on the left or not
* `join_right`: a boolean, whether the token should be joined to the token on the right or not
* `preserve`: a boolean, whether joiners and spacers can be attached to this token or not
* `features`: a list of string, the features attached to the token
* `spacer`: a boolean, whether the token is prefixed by a SentencePiece spacer or not (only set when using SentencePiece)
* `casing`: a `pyonmttok.Casing` value, the casing of the token (only set when tokenizing with `case_feature` or `case_markup`)

The `pyonmttok.TokenType` enumeration is used to identify tokens that were split by a subword tokenization. The enumeration has the following values:

* `TokenType.WORD`
* `TokenType.LEADING_SUBWORD`
* `TokenType.TRAILING_SUBWORD`

The `pyonmttok.Casing` enumeration is used to identify the original casing of a token that was lowercased by the `case_feature` or `case_markup` tokenization options. The enumeration has the following values:

* `Casing.LOWERCASE`
* `Casing.UPPERCASE`
* `Casing.MIXED`
* `Casing.CAPITALIZED`
* `Casing.NONE`

The `Tokenizer` instances provide methods to serialize or deserialize `Token` objects:

```python
# Serialize Token objects to strings that can be saved on disk.
tokenizer.serialize_tokens(tokens: List[pyonmttok.Token]) -> Tuple[List[str], List[List[str]]]

# Deserialize strings into Token objects.
tokenizer.deserialize_tokens(
    tokens: List[str],
    features: Optional[List[List[str]]] = None
) -> List[pyonmttok.Token]
```

## Utilities

### Interface

```python
# Returns True if the string has the placeholder format.
pyonmttok.is_placeholder(token: str)
```
