## [Unreleased]

### New features

### Fixes and improvements

* Fix compilations errors when SentencePiece is not installed

## [v1.10.1](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.10.1) (2018-10-08)

### Fixes and improvements

* Fix regression for SentencePiece: spacer annotation was not automatically enabled in tokenization mode "none"

## [v1.10.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.10.0) (2018-10-05)

### New features

* `CaseMarkup` flag to inject case information as new tokens

### Fixes and improvements

* Do not break compilation for users with old SentencePiece versions

## [v1.9.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.9.0) (2018-09-25)

### New features

* Vocabulary restriction for SentencePiece encoding

### Fixes and improvements

* Improve Tokenizer constructor for subword configuration

## [v1.8.4](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.8.4) (2018-09-24)

### Fixes and improvements

* Expose base methods in `Tokenizer` class
* Small performance improvements for standard use cases

## [v1.8.3](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.8.3) (2018-09-18)

### Fixes and improvements

* Fix count of Arabic characters in the map of detected alphabets

## [v1.8.2](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.8.2) (2018-09-10)

### Fixes and improvements

* Minor fix to CMakeLists.txt for SentencePiece compilation

## [v1.8.1](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.8.1) (2018-09-07)

### Fixes and improvements

* Support training SentencePiece as a subtokenizer

## [v1.8.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.8.0) (2018-09-07)

### New features

* Add learning interface for SentencePiece

## [v1.7.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.7.0) (2018-09-04)

### New features

* Add integrated Subword Learning with first support of BPE.

### Fixes and improvements

* Preserve placeholders as independent tokens for all modes

## [v1.6.2](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.6.2) (2018-08-29)

### New features

* Support SentencePiece sampling API

### Fixes and improvements

* Additional +30% speedup for BPE tokenization
* Fix BPE not respecting `PreserveSegmentedTokens` (#30)

## [v1.6.1](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.6.1) (2018-07-31)

### Fixes and improvements

* Fix Python package

## [v1.6.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.6.0) (2018-07-30)

### New features

* `PreserveSegmentedTokens` flag to not attach joiners or spacers to tokens segmented by any `Segment*` flags

### Fixes and improvements

* Do not rebuild `bpe_vocab` if already loaded (e.g. when `CacheModel` is set)

## [v1.5.3](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.5.3) (2018-07-13)

### Fixes and improvements

* Fix `PreservePlaceholders` with `JoinerAnnotate` that possibly modified other tokens

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
