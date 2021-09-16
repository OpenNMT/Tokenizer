import sys

if sys.platform == "win32":
    import ctypes
    import os

    import pkg_resources

    module_name = sys.modules[__name__].__name__
    package_dir = pkg_resources.resource_filename(module_name, "")

    add_dll_directory = getattr(os, "add_dll_directory", None)
    if add_dll_directory is not None:
        add_dll_directory(package_dir)

    for filename in os.listdir(package_dir):
        if filename.endswith(".dll"):
            ctypes.CDLL(os.path.join(package_dir, filename))

from ._ext import (
    BPELearner,
    Casing,
    SentencePieceLearner,
    SentencePieceTokenizer,
    SubwordLearner,
    Token,
    Tokenizer,
    TokenType,
    is_placeholder,
    set_random_seed,
)
