# Options

This file documents the options of the Tokenizer interface which can be used in:

* command line client
* C++ API
* Python API

*The exact name format of each option may be different depending on the API used.*

## General options

### `mode` (string, required)

Defines the tokenization mode:

* `conservative`: standard OpenNMT tokenization
* `aggressive`: standard OpenNMT tokenization but only keep sequences of letters/numbers (e.g. splits "2,000" to "2 , 0000")
* `char`: character tokenization
* `space`: space tokenization
* `none`: no tokenization is applied and the input is passed directly to the BPE or SP model if set.

`space` and `none` modes are incompatible with options listed in the *Segmenting* section.

### `case_feature` (boolean, default: `false`)

Lowercase text input and attach case information.

### `no_substitution` (boolean, default: `false`)

Disable substitution of special characters defined by the Tokenizer and found in the input text (e.g. joiners, spacers, etc.).

## Subtokenizer

### `bpe_model` (string, default: `""`)

Path to the BPE model trained with OpenNMT's `learn_bpe.lua` or the standard `learn_bpe.py`.

### `bpe_vocab` (string, default: `""`)

Path to the vocabulary file produced by `get_vocab.py`. If set, any merge operations that produce an OOV will be reverted.

### `bpe_vocab_threshold` (int, default: `50`)

When using `bpe_vocab`, any word with a frequency < `bpe_vocab_threshold` will be treated as OOV.

### `sp_model` (string, default: `""`)

Path to the SentencePiece model. To replicate `spm_encode`, the tokenization mode should be `none`.

## Marking joint tokens

These options inject characters to make the tokenization reversible.

### `joiner_annotate` (boolean, default: `false`)

Mark joints with joiner characters (mutually exclusive with `spacer_annotate`).

### `joiner` (string, default: `￭`)

When using `joiner_annotate`, the joiner to use.

### `joiner_new` (boolean, default: `false`)

When using `joiner_annotate`, make joiners independent tokens.

### `spacer_annotate` (boolean, default: `false`)

Mark spaces with spacer tokens (mutually exclusive with `joiner_annotate`)

### `spacer_new` (boolean, default: `false`)

When using `spacer_annotate`, make spacers independent tokens.

### `preserve_placeholders` (boolean, default: `false`)

Do not attach joiners or spacers to placeholders (character sequences encapsulated with ｟ and ｠).

## Segmenting

### `segment_case` (boolean, default: `false`)

Split token on case change.

### `segment_numbers` (boolean, default: `false`)

Split numbers into single digits.

### `segment_alphabet` (list of strings, default: `[]`)

List of alphabetss for which to split all letters. A complete list of supported alphabet is available in the source file `Alphabet.h`.

### `segment_alphabet_change` (boolean, default: `false`)

Split token on alphabet change.
