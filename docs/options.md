# Options

This file documents the options of the Tokenizer interface which can be used in:

* command line client
* C++ API
* Python API

*The exact name format of each option may be different depending on the API used.*

## General options

### `mode`

Defines the tokenization mode:

* `conservative`: standard OpenNMT tokenization
* `aggressive`: standard OpenNMT tokenization but only keep sequences of letters/numbers (e.g. splits "2,000" to "2 , 0000")
* `char`: character tokenization
* `space`: space tokenization
* `none`: no tokenization is applied and the input is passed directly to the BPE or SP model if set.

`space` and `none` modes are incompatible with options listed in the *Segmenting* section.

### `case_feature`

Lowercase text input and attach case information.

### `no_substitution`

Disable substitution of special characters defined by the Tokenizer and found in the input text (e.g. joiners, spacers, etc.).

## Subtokenizer

### `bpe_model`

Path to the BPE model trained with OpenNMT's `learn_bpe.lua` or the standard `learn_bpe.py`.

### `bpe_vocab`

Path to the vocabulary file produced by `get_vocab.py`. If set, any merge operations that produce an OOV will be reverted.

### `bpe_vocab_threshold`

When using `bpe_vocab`, any word with a frequency < `bpe_vocab_threshold` will be treated as OOV.

### `sp_model`

Path to the SentencePiece model. To replicate `spm_encode`, the tokenization mode should be `none`.

## Marking joint tokens

These options inject characters to make the tokenization reversible.

### `joiner_annotate`

Mark joints with joiner characters (mutually exclusive with `spacer_annotate`).

### `joiner_new`

When using `joiner_annotate`, make joiners independent tokens.

### `spacer_annotate`

Mark spaces with spacer tokens (mutually exclusive with `joiner_annotate`)

### `spacer_new`

When using `spacer_annotate`, make spacers independent tokens.

### `preserve_placeholders`

Do not attach joiners or spacers to placeholders (character sequences encapsulated with ｟ and ｠).

## Segmenting

### `segment_case`

Split token on case change.

### `segment_numbers`

Split numbers into single digits.

### `segment_alphabet`

List of alphabetss for which to split all letters. A complete list of supported alphabet is available in the source file `Alphabet.h`.

### `segment_alphabet_change`

Split token on alphabet change.
