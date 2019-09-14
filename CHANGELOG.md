**Notes on versioning:**

The project follows [semantic versioning 2.0.0](https://semver.org/). The API covers the following symbols:

* C++
  * `onmt::BPELearner`
  * `onmt::BPE`
  * `onmt::SPMLearner`
  * `onmt::SentencePiece`
  * `onmt::SpaceTokenizer`
  * `onmt::Tokenizer`
  * `onmt::unicode::*`
* Python
  * `pyonmttok.BPELearner`
  * `pyonmttok.SentencePieceLearner`
  * `pyonmttok.Tokenizer`

---

## [Unreleased]

### New features

### Fixes and improvements

## [v1.15.4](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.15.4) (2019-09-14)

### Fixes and improvements

* [Python] Fix possible runtime error on program exit when using `SentencePieceLearner`

## [v1.15.3](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.15.3) (2019-09-13)

### Fixes and improvements

* Fix possible memory issues when run in multiple threads with ICU

## [v1.15.2](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.15.2) (2019-09-11)

### Fixes and improvements

* [Python] Improve error checking in file based functions

## [v1.15.1](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.15.1) (2019-09-05)

### Fixes and improvements

* Fix regression in space tokenization: characters inside placeholders were incorrectly normalized

## [v1.15.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.15.0) (2019-09-05)

### New features

* `support_prior_joiners` flag to support tokenizing a pre-tokenized input

### Fixes and improvements

* Fix case markup when joiners or spacers are individual tokens

## [v1.14.1](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.14.1) (2019-08-07)

### Fixes and improvements

* Improve error checking

## [v1.14.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.14.0) (2019-07-19)

### New features

* [C++] Method to detokenize from `AnnotatedToken`s

### Fixes and improvements

* [Python] Release the GIL in time consuming functions (e.g. file tokenization, subword learning, etc.)
* Performance improvements

## [v1.13.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.13.0) (2019-06-12)

### New features

* [Python] File-based tokenization and detokenization APIs
* Support tokenizing files with multiple threads

### Fixes and improvements

* Respect "NoSubstitution" flag for combining marks applied on spaces

## [v1.12.1](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.12.1) (2019-05-27)

### Fixes and improvements

* Fix Python package

## [v1.12.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.12.0) (2019-05-27)

### New features

* Python API for subword learning (BPE and SentencePiece)
* C++ tokenization method to get the intermediate token representation

### Fixes and improvements

* Replace Boost.Python by pybind11 for the Python wrapper
* Fix verbose flag for SentencePiece training
* Check and raise possible errors during SentencePiece training

## [v1.11.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.11.0) (2019-02-05)

### New features

* Support copy operators on the Python client
* Support returning token locations in detokenized text

### Fixes and improvements

* Hide SentencePiece dependency in public headers

## [v1.10.6](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.10.6) (2019-01-15)

### Fixes and improvements

* Update SentencePiece to 0.1.8 in the Python package
* Allow naming positional arguments in the Python API

## [v1.10.5](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.10.5) (2019-01-03)

### Fixes and improvements

* More strict handle of combining marks - fixes #57 and #58

## [v1.10.4](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.10.4) (2018-12-18)

### Fixes and improvements

* Harden detokenization on invalid case markups combination

## [v1.10.3](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.10.3) (2018-11-05)

### Fixes and improvements

* Fix case markup for 1 letter words

## [v1.10.2](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.10.2) (2018-10-18)

### Fixes and improvements

* Fix compilations errors when SentencePiece is not installed
* Fix DLLs builds using Visual Studio
* Handle rare cases where SentencePiece returns 0 pieces

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
