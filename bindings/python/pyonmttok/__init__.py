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

    for library in ("icudt*", "icuuc*", "OpenNMTTokenizer"):
        library_path = glob.glob(os.path.join(package_dir, "%s.dll" % library))[0]
        ctypes.CDLL(library_path)

from pyonmttok._ext import (
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
from pyonmttok.version import __version__
