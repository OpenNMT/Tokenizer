## Python bindings

### Installation

```bash
pip install pyonmttok
```

### API

```python
import pyonmttok

tokenizer = pyonmt.Tokenizer(
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

tokens, features = tokenizer.tokenize(test: str)

text = tokenizer.detokenize(tokens, features)
text = tokenizer.detokenize(tokens)  # will fail if case_feature is set.
```

See the [documentation](../../docs/options.md) for a description of each option.
