import sys

if sys.platform == "win32":
    import ctypes
    import glob
    import os

    import pkg_resources

    module_name = sys.modules[__name__].__name__
    package_dir = pkg_resources.resource_filename(module_name, "")

    add_dll_directory = getattr(os, "add_dll_directory", None)
    if add_dll_directory is not None:
        add_dll_directory(package_dir)

    for library in glob.glob(os.path.join(package_dir, "*.dll")):
        ctypes.CDLL(library)

from pyonmttok._ext import (
    BPELearner,
    Casing,
    SentencePieceLearner,
    SentencePieceTokenizer,
    SubwordLearner,
    Token,
    Tokenizer,
    TokenType,
    Vocab,
    is_placeholder,
    is_valid_language,
    set_random_seed,
)
from pyonmttok.version import __version__


def build_vocab_from_tokens(
    tokens,
    maximum_size=0,
    minimum_frequency=1,
    special_tokens=None,
):
    vocab = Vocab(special_tokens)
    for token in tokens:
        vocab.add_token(token)
    vocab.resize(maximum_size=maximum_size, minimum_frequency=minimum_frequency)
    return vocab


def build_vocab_from_lines(
    lines,
    tokenizer=None,
    maximum_size=0,
    minimum_frequency=1,
    special_tokens=None,
):
    vocab = Vocab(special_tokens)
    for line in lines:
        vocab.add_from_text(line.rstrip("\r\n"), tokenizer)
    vocab.resize(maximum_size=maximum_size, minimum_frequency=minimum_frequency)
    return vocab
