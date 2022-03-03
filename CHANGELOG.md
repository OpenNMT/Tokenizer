**Notes on versioning:**

The project follows [semantic versioning 2.0.0](https://semver.org/). The API covers the following symbols:

* C++
  * `onmt::BPELearner`
  * `onmt::BPE`
  * `onmt::SPMLearner`
  * `onmt::SentencePiece`
  * `onmt::SpaceTokenizer`
  * `onmt::Tokenizer`
  * `onmt::Vocab`
  * `onmt::unicode::*`
* Python
  * `pyonmttok.BPELearner`
  * `pyonmttok.SentencePieceLearner`
  * `pyonmttok.SentencePieceTokenizer`
  * `pyonmttok.Tokenizer`
  * `pyonmttok.Vocab`

---

## [Unreleased]

### New features

### Fixes and improvements

## [v1.30.1](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.30.1) (2022-01-25)

### Fixes and improvements

* Fix deprecated languages codes in ICU that are incorrectly considered as invalid (e.g. "tl" for Tagalog)

## [v1.30.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.30.0) (2021-11-29)

### New features

* [Python] Build wheels for AArch64 Linux

### Fixes and improvements

* [Python] Update ICU to 70.1

## [v1.29.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.29.0) (2021-10-08)

### Changes

* [Python] Drop support for Python 3.5

### New features

* [Python] Build wheels for Python 3.10
* [Python] Add tokenization method `Tokenizer.tokenize_batch`

## [v1.28.1](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.28.1) (2021-09-30)

### Fixes and improvements

* Fix detokenization when a token includes a fullwidth percent sign (％) that is not used as an escape sequence (version 1.27.0 contained a partial fix for this bug)

## [v1.28.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.28.0) (2021-09-17)

### Changes

* [C++] Remove the `SpaceTokenizer` class that is not meant to be public and can be confused with the "space" tokenization mode

### New features

* Build Python wheels for Windows
* Add option `tokens_delimiter` to configure how tokens are delimited in tokenized files (default is a space)
* Expose option `with_separators` in Python and CLI to include whitespace characters in the tokenized output
* [Python] Add package version information in `pyonmttok.__version__`

### Fixes and improvements

* Fix detokenization when option `with_separators` is enabled

## [v1.27.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.27.0) (2021-08-30)

### Changes

* Linux Python wheels are now compiled with `manylinux2010` and require `pip` >= 19.0 for installation
* macOS Python wheels now require macOS >= 10.14

### Fixes and improvements

* Fix casing resolution when some letters do not have case information
* Fix detokenization when a token includes a fullwidth percent sign (％) that is not used as an escape sequence
* Improve error message when setting invalid `segment_alphabet` or `lang` options
* Update SentencePiece to 0.1.96
* [Python] Improve declaration of functions and classes for better type hints and checks
* [Python] Update ICU to 69.1

## [v1.26.4](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.26.4) (2021-06-25)

### Fixes and improvements

* Fix regression introduced in last version for preserved tokens that are not segmented by BPE

## [v1.26.3](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.26.3) (2021-06-24)

### Fixes and improvements

* Fix another divergence with the SentencePiece output when there is only one subword and the spacer is detached

## [v1.26.2](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.26.2) (2021-06-08)

### Fixes and improvements

* Fix a divergence with the SentencePiece output when the spacer is detached from the word

## [v1.26.1](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.26.1) (2021-05-31)

### Fixes and improvements

* Fix application of the BPE vocabulary when using `preserve_segmented_tokens` and a subword appears without joiner in the vocabulary
* Fix compilation with ICU versions older than 60

## [v1.26.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.26.0) (2021-04-19)

### New features

* Add `lang` tokenization option to apply language-specific case mappings

### Fixes and improvements

* Use ICU to convert strings to Unicode values instead of a custom implementation

## [v1.25.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.25.0) (2021-03-15)

### New features

* Add `training` flag in tokenization methods to disable subword regularization during inference
* [Python] Implement `__len__` method in the `Token` class

### Fixes and improvements

* Raise an error when enabling `case_markup` with incompatible tokenization modes "space" and "none"
* [Python] Improve parallelization when `Tokenizer.tokenize` is called from multiple Python threads (the Python GIL is now released)
* [Python] Cleanup some manual Python <-> C++ types conversion

## [v1.24.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.24.0) (2021-02-16)

### New features

* Add `verbose` flag in file tokenization APIs to log progress every 100,000 lines
* [Python] Add `options` property to `Tokenizer` instances
* [Python] Add class `pyonmttok.SentencePieceTokenizer` to help creating a tokenizer compatible with SentencePiece

### Fixes and improvements

* Fix deserialization into `Token` objects that was sometimes incorrect
* Fix Windows compilation
* Fix Google Test integration that was sometimes installed as part of `make install`
* [Python] Update pybind11 to 2.6.2
* [Python] Update ICU to 66.1
* [Python] Compile ICU with optimization flags

## [v1.23.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.23.0) (2020-12-30)

### Changes

* Drop Python 2 support

### New features

* Publish Python wheels for macOS

### Fixes and improvements

* Improve performance in all tokenization modes (up to 2x faster)
* Fix missing space escaping within protected sequences in "none" and "space" tokenization modes
* Fix a regression introduced in 1.20 where `segment_alphabet_*` options behave differently on characters that appear in multiple Unicode scripts (e.g. some Japanese characters can belong to both Hiragana and Katakana scripts and should not trigger a segmentation)
* Fix a regression introduced in 1.21 where a joiner is incorrectly placed when using `preserve_segmented_tokens` and the word is segmented by both a `segment_*` option and BPE
* Fix incorrect tokenization when using `support_prior_joiners` and some joiners are within protected sequences

## [v1.22.2](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.22.2) (2020-11-12)

### Fixes and improvements

* Do not require "none" tokenization mode for SentencePiece vocabulary restriction

## [v1.22.1](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.22.1) (2020-10-30)

### Fixes and improvements

* Fix error when enabling vocabulary restriction with SentencePiece and `spacer_annotate` is not explicitly set
* Fix backward compatibility with Kangxi and Kanbun scripts (see `segment_alphabet` option)

## [v1.22.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.22.0) (2020-10-29)

### Changes

* [C++] Subword model caching is no longer supported and should be handled by the client. The subword encoder instance can now be passed as a `std::shared_ptr` to make it outlive the `Tokenizer` instance.

### New features

* Add `set_random_seed` function to make subword regularization reproducible
* [Python] Support serialization of `Token` instances
* [C++] Add `Options` structure to configure tokenization options (`Flags` can still be used for backward compatibility)

### Fixes and improvements

* Fix BPE vocabulary restriction when using `joiner_new`, `spacer_annotate`, or `spacer_new` (the previous implementation always assumed `joiner_annotate` was used)
* [Python] Fix `spacer` argument name in `Token` constructor
* [C++] Fix ambiguous subword encoder ownership by using a `std::shared_ptr`

## [v1.21.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.21.0) (2020-10-22)

### New features

* Accept vocabularies with tab-separated frequencies (format produced by SentencePiece)

### Fixes and improvements

* Fix BPE vocabulary restriction when words have a leading or trailing joiner
* Raise an error when using a multi-character joiner and `support_prior_joiner`
* [Python] Implement `__hash__` method of `pyonmttok.Token` objects to be consistent with the `__eq__` implementation
* [Python] Declare `pyonmttok.Tokenizer` arguments (except `mode`) as keyword-only
* [Python] Improve compatibility with Python 3.9

## [v1.20.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.20.0) (2020-09-24)

### Changes

* The following changes affect users compiling the project from the source. They ensure users get the best performance and all features by default:
  * ICU is now required to improve performance and Unicode support
  * SentencePiece is now integrated as a Git submodule and linked statically to the project
  * Boost is no longer required, the project now uses [cxxopts](https://github.com/jarro2783/cxxopts) which is integrated as a Git submodule
  * The project is compiled in `Release` mode by default
  * Tests are no longer compiled by default (use `-DBUILD_TESTS=ON` to compile the tests)

### New features

* Accept any [Unicode script aliases](https://en.wikipedia.org/wiki/Script_(Unicode)#List_of_scripts_in_Unicode) in the `segment_alphabet` option
* Update SentencePiece to 0.1.92
* [Python] Improve the capabilities of the `Token` class:
  * Implement the `__repr__` method
  * Allow setting all attributes in the constructor
  * Add a copy constructor
* [Python] Add a copy constructor for the `Tokenizer` class

### Fixes and improvements

* [Python] Accept `None` value for `segment_alphabet` argument

## [v1.19.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.19.0) (2020-09-02)

### New features

* Add BPE dropout ([Provilkov et al. 2019](https://www.aclweb.org/anthology/2020.acl-main.170/))
* [Python] Introduce the "Token API": a set of methods that manipulate `Token` objects instead of serialized strings
* [Python] Add `unicode_ranges` argument to the `detokenize_with_ranges` method to return ranges over Unicode characters instead of bytes

### Fixes and improvements

* Include "Half-width kana" in Katakana script detection

## [v1.18.5](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.18.5) (2020-07-07)

### Fixes and improvements

* Fix possible crash when applying a case insensitive BPE model on Unicode characters

## [v1.18.4](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.18.4) (2020-05-22)

### Fixes and improvements

* Fix segmentation fault on `cli/tokenize` exit
* Ignore empty tokens during detokenization
* When writing to a file, avoid flushing the output stream on each line
* Update `cli/CMakeLists.txt` to mark Boost.ProgramOptions as required

## [v1.18.3](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.18.3) (2020-03-09)

### Fixes and improvements

* Strip token annotations when calling `SubwordLearner.ingest_token`

## [v1.18.2](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.18.2) (2020-02-17)

### Fixes and improvements

* Speed and memory improvements for BPE learning

## [v1.18.1](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.18.1) (2020-01-16)

### Fixes and improvements

* [Python] Fix memory leak when deleting Tokenizer object

## [v1.18.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.18.0) (2020-01-06)

### New features

* Include `is_placeholder` function in the Python API
* Add `ingest_token` method to learner objects to allow external tokenization

## [v1.17.2](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.17.2) (2019-12-06)

### Fixes and improvements

* Fix joiner annotation when SentencePiece returns isolated spacers
* Apply `preserve_segmented_tokens` in "none" tokenization mode
* Performance improvements when using `case_feature` or `case_markup`
* Add missing `--no_substitution` flag on the command line client

## [v1.17.1](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.17.1) (2019-11-28)

### Fixes and improvements

* Fix missing case features for isolated joiners or spacers

## [v1.17.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.17.0) (2019-11-13)

### New features

* Flag `soft_case_regions` to minimize the number of uppercase regions when using `case_markup`

### Fixes and improvements

* Fix mismatch between subword learning and encoding when using `case_feature`
* [C++] Fix missing default value for new argument of constructor `SPMLearner`

## [v1.16.1](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.16.1) (2019-10-21)

### Fixes and improvements

* Fix invalid SentencePiece training file when generated with `SentencePieceLearner.ingest` (newlines were missing)
* Correctly ignore placeholders when using `SentencePieceLearner` without a tokenizer

## [v1.16.0](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.16.0) (2019-10-07)

### New features

* Support keeping the vocabulary generated by SentencePiece with the `keep_vocab` argument
* [C++] Add intermediate method to annotate tokens before detokenization

### Fixes and improvements

* Improve file read/write errors detection
* [Python] Lower the risk of ABI incompatibilities with other pybind11 extensions

## [v1.15.7](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.15.7) (2019-09-20)

### Fixes and improvements

* Do not apply case modifiers on placeholder tokens

## [v1.15.6](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.15.6) (2019-09-16)

### Fixes and improvements

* Fix placeholder tokenization when followed by a combining mark

## [v1.15.5](https://github.com/OpenNMT/Tokenizer/releases/tag/v1.15.5) (2019-09-16)

### Fixes and improvements

* [Python] Downgrade `pybind11` to fix segmentation fault when importing after non-compliant Python wheels

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
