# Tokenization options

This file documents the options of the Tokenizer interface which can be used in:

* command line client
* C++ API
* Python API

*The exact name format of each option may be different depending on the API used.*

## Terminology

* **joiner**: special character indicating that the surrounding tokens should be merged when detokenized
* **spacer**: special character indicating that a space should be introduced when detokenized
* **placeholder** (or **protected sequence**): sequence of characters delimited by ｟ and ｠ that should not be segmented

## General

### `mode` (string, required)

Defines the tokenization mode:

* `conservative`: standard OpenNMT tokenization
* `aggressive`: standard OpenNMT tokenization but only keep sequences of the same character type (e.g. "2,000" is tokenized to "2 , 000", "soft-landing" to "soft - landing", etc.)
* `char`: character tokenization
* `space`: space tokenization
* `none`: no tokenization is applied and the input is passed directly to the BPE or SentencePiece model if set.

```bash
$ echo "It costs £2,000." | cli/tokenize --mode conservative
It costs £ 2,000 .

$ echo "It costs £2,000." | cli/tokenize --mode aggressive
It costs £ 2 , 000 .

$ echo "It costs £2,000." | cli/tokenize --mode char
I t c o s t s £ 2 , 0 0 0 .

$ echo "It costs £2,000." | cli/tokenize --mode space
It costs £2,000.

$ echo "It costs £2,000." | cli/tokenize --mode none
It costs £2,000.
```

**Notes:**

* `space` and `none` modes are incompatible with options listed in the *Segmenting* section.
* In all modes, the text is at least segmented on placeholders:

```bash
$ echo "a｟b｠c" | cli/tokenize --mode space
a ｟b｠ c

$ echo "a｟b｠c" | cli/tokenize --mode none
a ｟b｠ c
```

### `no_substitution` (boolean, default: `false`)

Disable substitution of special characters defined by the Tokenizer and found in the input text (e.g. joiners, spacers, etc.).

## Case annotation

### `case_feature` (boolean, default: `false`)

Lowercase text input and attach case information with the special separator ￨. In the Python and C++ APIs, the case feature is returned in a separate data structure.

```bash
$ echo "Hello world!" | cli/tokenize --case_feature
hello￨C world￨L !￨N
```

Possible case types:

* `C`: capitalized
* `L`: lowercase
* `U`: uppercase
* `M`: mixed
* `N`: none

The case is restored after detokenization:

```bash
$ echo 'hello￨C world￨L !￨N' | cli/detokenize --case_feature
Hello world !
```

### `case_markup` (boolean, default: `false`)

Lowercase text input and inject case markups as additional tokens. This option also enables `segment_case`.

```bash
$ echo "Hello world!" | cli/tokenize --case_markup
｟mrk_case_modifier_C｠ hello world !

$ echo "Hello WORLD!" | cli/tokenize --case_markup
｟mrk_case_modifier_C｠ hello ｟mrk_begin_case_region_U｠ world ｟mrk_end_case_region_U｠ !

$ echo "Hello WOrld!" | cli/tokenize --case_markup
｟mrk_case_modifier_C｠ hello ｟mrk_begin_case_region_U｠ wo ｟mrk_end_case_region_U｠ rld !

$ echo "Hello WORLD!" | cli/tokenize --case_markup --bpe_model model.bpe
｟mrk_case_modifier_C｠ he llo ｟mrk_begin_case_region_U｠ wo rld ｟mrk_end_case_region_U｠ !
```

The case is restored after detokenization:

```bash
$ echo "｟mrk_case_modifier_C｠ hello world !" | cli/detokenize
Hello world !
```

### `soft_case_regions` (boolean, default: `false`)

With `case_markup`, allow uppercase regions to span over case invariant tokens to optimize the overall number of case regions.

```bash
$ echo "U.N" | cli/tokenize --case_markup
｟mrk_case_modifier_C｠ u. ｟mrk_case_modifier_C｠ n

$ echo "U.N" | cli/tokenize --case_markup --soft_case_regions
｟mrk_begin_case_region_U｠ u. n ｟mrk_end_case_region_U｠

$ echo "A-BC/D" | cli/tokenize --case_markup
｟mrk_case_modifier_C｠ a- ｟mrk_begin_case_region_U｠ bc ｟mrk_end_case_region_U｠ / ｟mrk_case_modifier_C｠ d

$ echo "A-BC/D" | cli/tokenize --case_markup --soft_case_regions
｟mrk_begin_case_region_U｠ a- bc / d ｟mrk_end_case_region_U｠
```

## Subword encoding

### `bpe_model_path` (string, default: `""`)

Path to the BPE model.

### `sp_model_path` (string, default: `""`)

Path to the SentencePiece model.

To replicate `spm_encode`, the tokenization mode should be `none`. If another mode is selected, SentencePiece will be used as a subtokenizer and receive tokens as inputs.

```bash
$ echo "Hello world!" | cli/tokenize --mode none --sp_model_path wmtende.model
▁H ello ▁world !

$ echo "Hello world!" | cli/tokenize --mode none --sp_model_path wmtende.model --joiner_annotate
H ￭ello world ￭!
```

### `sp_nbest_size` (int, default: `0`)

Number of candidates for the SentencePiece sampling API. When the value is 0, the standard SentencePiece encoding is used.

```bash
$ echo "Hello world!" | cli/tokenize --mode none --sp_model_path wmtende.model --sp_nbest_size 64
▁H e llo ▁world !

$ echo "Hello world!" | cli/tokenize --mode none --sp_model_path wmtende.model --sp_nbest_size 64
▁H el l o ▁world !
```

### `sp_alpha` (float, default: `0.1`)

Smoothing parameter for the SentencePiece sampling API.

### `vocabulary_path` (string, default: `""`)

Path to the vocabulary file. If set, subword encoders will only generate tokens that exist in the vocabulary. Format is: `<token> <space> <frequency>`. A missing frequency is equivalent to 1.

### `vocabulary_threshold` (int, default: `0`)

When using `vocabulary`, any words with a frequency lower than `vocabulary_threshold` will be treated as OOV.

## Reversible tokenization

These options inject special characters to make the tokenization reversible.

### `joiner_annotate` (boolean, default: `false`)

Mark joints with joiner characters (mutually exclusive with `spacer_annotate`). When possible, joiners are attached to the least important token.

```bash
$ echo "Hello World!" | cli/tokenize --joiner_annotate
Hello World ￭!

$ echo "It costs £2,000." | cli/tokenize --mode aggressive --joiner_annotate
It costs £￭ 2 ￭,￭ 000 ￭.
```
### `joiner` (string, default: `￭`)

When using `joiner_annotate`, the joiner to use.

```bash
$ echo "Hello World!" | cli/tokenize --joiner_annotate --joiner @@
Hello World @@!
```

### `joiner_new` (boolean, default: `false`)

When using `joiner_annotate`, make joiners independent tokens.

```bash
$ echo "Hello World!" | cli/tokenize --joiner_annotate --joiner_new
Hello World ￭ !
```

### `spacer_annotate` (boolean, default: `false`)

Mark spaces with spacer tokens (mutually exclusive with `joiner_annotate`)

```bash
$ echo "Hello World!" | cli/tokenize --spacer_annotate
Hello ▁World !
```

### `spacer_new` (boolean, default: `false`)

When using `spacer_annotate`, make spacers independent tokens.

```bash
$ echo "Hello World!" | cli/tokenize --spacer_annotate --spacer_new
Hello ▁ World !

$ echo "Hello World!" | cli/tokenize --spacer_annotate --spacer_new --mode char
H e l l o ▁ W o r l d !
```

### `preserve_placeholders` (boolean, default: `false`)

Do not attach joiners or spacers to placeholders.

```bash
$ echo "Hello｟World｠" | cli/tokenize --joiner_annotate
Hello ￭｟World｠

$ echo "Hello｟World｠" | cli/tokenize --joiner_annotate --preserve_placeholders
Hello ￭ ｟World｠
```

### `preserve_segmented_tokens` (boolean, default: `false`)

Do not attach joiners or spacers to tokens that were segmented by:

* a `segment_*` option (see next section)

```bash
$ echo "測試abc" | cli/tokenize --segment_alphabet Han --segment_alphabet_change \
    --joiner_annotate
測￭ 試￭ abc

$ echo "測試abc" | cli/tokenize --segment_alphabet Han --segment_alphabet_change \
    --joiner_annotate --preserve_segmented_tokens
測 ￭ 試 ￭ abc
```

* the tokenization mode "none"

```bash
$ echo "a｟b｠" | cli/tokenize --mode none --joiner_annotate
a￭ ｟b｠

$ echo "a｟b｠" | cli/tokenize --mode none --joiner_annotate --preserve_segmented_tokens
a ￭ ｟b｠
```

### `support_prior_joiners`(boolean, default: `false`)

If the input already has joiners, support these joiners as pre-tokenization marks.

```bash
$ echo "pre￭ tokenization." | cli/tokenize --joiner_annotate --support_prior_joiners \
   --mode aggressive
pre￭ tokenization ￭.
```

## Segmenting

### `segment_case` (boolean, default: `false`)

Split token on case change.

```bash
$ echo "WiFi" | cli/tokenize --segment_case
Wi Fi
```

### `segment_numbers` (boolean, default: `false`)

Split numbers into single digits (requires the `aggressive` tokenization mode).

```bash
$ echo "1234" | cli/tokenize --mode aggressive --segment_numbers
1 2 3 4
```

### `segment_alphabet` (list of strings, default: `[]`)

List of alphabets for which to split all letters. A complete list of supported alphabets is available in the source file [`Alphabet.h`](../include/onmt/Alphabet.h).

```bash
$ echo "測試 abc" | cli/tokenize --segment_alphabet Han
測 試 abc

$ echo "測試 abc" | cli/tokenize --segment_alphabet Han,Latin
測 試 a b c
```

### `segment_alphabet_change` (boolean, default: `false`)

Split token on alphabet change.

```bash
$ echo "測試abc" | cli/tokenize --segment_alphabet_change
測試 abc
```
