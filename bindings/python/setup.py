import os
import sys

from setuptools import setup, Extension


include_dirs = []
library_dirs = []

def _maybe_add_library_root(lib_name, header_only=False):
  if "%s_ROOT" % lib_name in os.environ:
    root = os.environ["%s_ROOT" % lib_name]
    include_dirs.append("%s/include" % root)
    if not header_only:
      library_dirs.append("%s/lib" % root)

_maybe_add_library_root("BOOST")
_maybe_add_library_root("TOKENIZER")
_maybe_add_library_root("SENTENCEPIECE", header_only=True)

tokenizer_module = Extension(
    "pyonmttok.tokenizer",
    sources=["Python.cc"],
    extra_compile_args=["-std=c++11"],
    include_dirs=include_dirs,
    library_dirs=library_dirs,
    libraries=[
        os.getenv("BOOST_PYTHON_LIBRARY",
                  "boost_python%d%d" % (sys.version_info[0], sys.version_info[1])),
        "OpenNMTTokenizer"])

setup(
    name="pyonmttok",
    version="1.10.1",
    license="MIT",
    description="OpenNMT tokenization library",
    author="OpenNMT",
    author_email="guillaume.klein@opennmt.net",
    url="http://opennmt.net",
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python",
        "Topic :: Text Processing :: Linguistic",
        "Topic :: Software Development :: Libraries :: Python Modules"
    ],
    project_urls={
        "Forum": "http://forum.opennmt.net/",
        "Source": "https://github.com/OpenNMT/Tokenizer/"
    },
    packages=["pyonmttok"],
    ext_modules=[tokenizer_module]
)
