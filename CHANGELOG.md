## [Unreleased]

### Breaking changes

### New features

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
