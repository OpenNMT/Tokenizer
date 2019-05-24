# Python bindings

```bash
pip install pyonmttok
```

## Tokenization

```python
import pyonmttok

tokenizer = pyonmttok.Tokenizer(
    mode: str,
    bpe_model_path="",
    bpe_vocab_path="",  # Deprecated, use "vocabulary_path" instead.
    bpe_vocab_threshold=50,  # Deprecated, use "vocabulary_threshold" instead.
    vocabulary_path="",
    vocabulary_threshold=0,
    sp_model_path="",
    sp_nbest_size=0,
    sp_alpha=0.1,
    joiner="￭",
    joiner_annotate=False,
    joiner_new=False,
    spacer_annotate=False,
    spacer_new=False,
    case_feature=False,
    case_markup=False,
    no_substitution=False,
    preserve_placeholders=False,
    preserve_segmented_tokens=False,
    segment_case=False,
    segment_numbers=False,
    segment_alphabet_change=False,
    segment_alphabet=[])

tokens, features = tokenizer.tokenize(text: str)

text = tokenizer.detokenize(tokens, features)
text = tokenizer.detokenize(tokens)  # will fail if case_feature is set.

# Function that also returns a dictionary mapping a token index to a range in
# the detokenized text. Set merge_ranges=True to merge consecutive ranges, e.g.
# subwords of the same token in case of subword tokenization.
text, ranges = tokenizer.detokenize_with_ranges(tokens, merge_ranges=True)
```

See the [documentation](../../docs/options.md) for a description of each option.

## Subword learning

The Python wrapper supports BPE and SentencePiece subword learning through a common interface:

1\. (optional) Create the `pyonmttok.Tokenizer` that you want to apply, e.g.:

```python
tokenizer = pyonmttok.Tokenizer(
    "aggressive", joiner_annotate=True, segment_numbers=True)
```

2\. Create the subword learner, e.g.:

```python
learner = pyonmttok.BPELearner(tokenizer=tokenizer, symbols=32000)
# or:
learner = pyonmttok.SentencePieceLearner(vocab_size=32000, character_coverage=0.98)
```

3\. Feed some raw data:

```python
learner.ingest("Hello world!")
learner.ingest_file("/data/train.en")
```

4\. Start the learning process:

```python
tokenizer = learner.learn("/data/model-32k")
```

The returned `tokenizer` instance can be used to apply subword tokenization on new data.

### Interface

```python
# See https://github.com/rsennrich/subword-nmt/blob/master/subword_nmt/learn_bpe.py
# for argument documentation.
learner = pyonmttok.BPELearner(
    tokenizer=None,  # Defaults to tokenization mode "space".
    symbols=10000,
    min_frequency=2,
    total_symbols=False,
    dict_path="")

# See https://github.com/google/sentencepiece/blob/master/src/spm_train_main.cc
# for available training options.
learner = pyonmttok.SentencePieceLearner(
    tokenizer=None,  # Defaults to tokenization mode "none".
    **training_options)

learner.ingest(text: str)
learner.ingest_file(path: str)

tokenizer = learner.learn(model_path: str, verbose=False)
```
