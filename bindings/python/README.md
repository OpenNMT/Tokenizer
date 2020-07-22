# Python bindings

```bash
pip install pyonmttok
```

## Tokenization

```python
import pyonmttok

tokenizer = pyonmttok.Tokenizer(
    mode: str,
    bpe_model_path: str = "",
    vocabulary_path: str = "",
    vocabulary_threshold: int = 0,
    sp_model_path: str = "",
    sp_nbest_size: int = 0,
    sp_alpha: float = 0.1,
    joiner: str = "￭",
    joiner_annotate: bool = False,
    joiner_new: bool = False,
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
    support_prior_joiners: bool = False,
    segment_alphabet: list = [])

tokens, features = tokenizer.tokenize(text: str)

text = tokenizer.detokenize(tokens: list, features: list = None)

# Function that also returns a dictionary mapping a token index to a range in
# the detokenized text (in bytes, unless unicode_ranges is set).
# Set merge_ranges=True to merge consecutive ranges, e.g. subwords of the same
# token in case of subword tokenization.
text, ranges = tokenizer.detokenize_with_ranges(
    tokens: list,
    merge_ranges: bool = False,
    unicode_ranges: bool = False)

# File-based APIs
tokenizer.tokenize_file(input_path: str, output_path: str, num_threads: int = 1)
tokenizer.detokenize_file(input_path: str, output_path: str)
```

See the [documentation](../../docs/options.md) for a description of each option.

## Subword learning

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
    tokenizer: pyonmttok.Tokenizer = None,  # Defaults to tokenization mode "space".
    symbols: int = 10000,
    min_frequency: int = 2,
    total_symbols: bool = False)

# See https://github.com/google/sentencepiece/blob/master/src/spm_train_main.cc
# for available training options.
learner = pyonmttok.SentencePieceLearner(
    tokenizer: pyonmttok.Tokenizer = None,  # Defaults to tokenization mode "none".
    keep_vocab: bool = False,  # Keep the generated vocabulary (model_path will act like model_prefix in spm_train)
    **training_options)

learner.ingest(text: str)
learner.ingest_file(path: str)
learner.ingest_token(token: str)

tokenizer = learner.learn(model_path: str, verbose: bool = False)
```
