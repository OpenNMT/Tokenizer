## [Unreleased]

### New features

### Fixes and improvements

## [v1.5.2](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.5.2) (2018-07-12)

### Fixes and improvements

* Fix support of BPE models v0.2 trained with `learn_bpe.py`

## [v1.5.1](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.5.1) (2018-07-12)

### Fixes and improvements

* Do not escape spaces in placeholders value if `NoSubstitution` is enabled

## [v1.5.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.5.0) (2018-07-03)

### New features
* Support `apply_bpe.py` 0.3 mode

### Fixes and improvements

* Up to x3 faster tokenization and detokenization

## [v1.4.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.4.0) (2018-06-13)

### New features

* New character level tokenization mode `Char`
* Flag `SpacerNew` to make spacers independent tokens

### Fixes and improvements

* Replace spacer tokens by substitutes when found in the input text
* Do not enable spacers by default when SentencePiece is used as a subtokenizer

## [v1.3.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.3.0) (2018-04-07)

### New features

* New tokenization mode `None` that simply forwards the input text
* Support SentencePiece, as a tokenizer or sub-tokenizer
* Flag `PreservePlaceholders` to not mark placeholders with joiners or spacers

### Fixes and improvements

* Revisit Python compilation to support wheels building

## [v1.2.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.2.0) (2018-03-28)

### New features

* Add API to retrieve discovered alphabet during tokenization
* Flag to convert joiners to spacers

### Fixes and improvements

* Add install target for the Python bindings library

## [v1.1.1](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.1.1) (2018-01-23)

### Fixes and improvements

* Make `Alphabet.h` public

## [v1.1.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.1.0) (2018-01-22)

### New features

* Python bindings
* Tokenization flag to disable special characters substitution

### Fixes and improvements

* Fix incorrect behavior when `--segment_alphabet` is not set by the client
* Fix alphabet identification
* Fix segmentation fault when tokenizing empty string on spaces

## [v1.0.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.0.0) (2017-12-11)

### Breaking changes

* New `Tokenizer` constructor requiring bit flags

### New features

* Support BPE modes from `learn_bpe.lua`
* Case insensitive BPE models
* Space tokenization mode
* Alphabet segmentation
* Do not tokenize blocks encapsulated by `｟` and `｠`
* `segment_numbers` flag to split numbers into digits
* `segment_case` flag to split words on case changes
* `segment_alphabet_change` flag to split on alphabet change
* `cache_bpe_model` flag to cache BPE models for future instances

### Fixes and improvements

* Fix `SpaceTokenizer` crash with leading or trailing spaces
* Fix incorrect tokenization around tabulation character (#5)
* Fix incorrect joiner between numeric and punctuation

## [v0.2.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v0.2.0) (2017-03-08)

### New features

* Add CMake install rule
* Add API option to include separators
* Add static library compilation support

### Fixes and improvements

* Rename library to libOpenNMTTokenizer
* Make words features optional in tokenizer API
* Make `unicode` headers private

## [v0.1.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v0.1.0) (2017-02-14)

Initial release.
