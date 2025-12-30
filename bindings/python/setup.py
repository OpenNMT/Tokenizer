import os
import sys
import pybind11
from setuptools import Extension, find_packages, setup

include_dirs = [pybind11.get_include()]
library_dirs = []
libraries = []
extra_objects = []
extra_link_args = []


def _get_long_description():
    readme_path = "README.md"
    with open(readme_path, encoding="utf-8") as readme_file:
        return readme_file.read()


def _get_project_version():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    version_path = os.path.join(base_dir, "pyonmttok", "version.py")
    version = {}
    with open(version_path, encoding="utf-8") as fp:
        exec(fp.read(), version)
    return version["__version__"]


def _maybe_add_library_root(lib_name, header_only=False):
    root = os.environ.get("%s_ROOT" % lib_name)
    if root is None:
        return None
    include_dirs.append(os.path.join(root, "include"))
    if not header_only:
        for lib_subdir in ("lib64", "lib"):
            lib_dir = os.path.join(root, lib_subdir)
            if os.path.isdir(lib_dir):
                library_dirs.append(lib_dir)
                return lib_dir
    return root


tokenizer_root = _maybe_add_library_root("TOKENIZER")
icu_root = os.environ.get("ICU_ROOT")

# Handle static linking on Linux (manylinux)
if sys.platform.startswith("linux") and tokenizer_root and icu_root:
    print(f"Using static linking for Linux")
    print(f"TOKENIZER_ROOT: {tokenizer_root}")
    print(f"ICU_ROOT: {icu_root}")

    # Link statically against OpenNMTTokenizer
    tokenizer_lib = os.path.join(tokenizer_root, "lib", "libOpenNMTTokenizer.a")
    if os.path.exists(tokenizer_lib):
        print(f"Found tokenizer static lib: {tokenizer_lib}")
        extra_objects.append(tokenizer_lib)
    else:
        print(
            f"Tokenizer static lib not found at {tokenizer_lib}, using dynamic linking"
        )
        libraries.append("OpenNMTTokenizer")

    # Link statically against ICU libraries in correct order
    for lib_subdir in ("lib64", "lib"):
        icu_lib_dir = os.path.join(icu_root, lib_subdir)
        if os.path.isdir(icu_lib_dir):
            # ICU libraries must be linked in this specific order
            for icu_lib in ["icui18n", "icuuc", "icudata"]:
                icu_lib_path = os.path.join(icu_lib_dir, f"lib{icu_lib}.a")
                if os.path.exists(icu_lib_path):
                    print(f"Found ICU static lib: {icu_lib_path}")
                    extra_objects.append(icu_lib_path)
                else:
                    print(f"WARNING: ICU lib not found: {icu_lib_path}")
            break

    # Add necessary system libraries for static linking
    libraries.extend(["stdc++", "m", "dl", "pthread"])
else:
    # Dynamic linking for macOS and Windows
    libraries.append("OpenNMTTokenizer")

cflags = ["-std=c++17", "-fvisibility=hidden"]
ldflags = []
package_data = {}

if sys.platform == "darwin":
    cflags.append("-mmacosx-version-min=10.14")
    ldflags.append("-Wl,-rpath,/usr/local/lib")
elif sys.platform == "win32":
    cflags = ["/std:c++17", "/d2FH4-"]
    package_data["pyonmttok"] = ["*.dll"]

# Combine ldflags with extra_link_args
extra_link_args.extend(ldflags)

print(f"include_dirs: {include_dirs}")
print(f"library_dirs: {library_dirs}")
print(f"libraries: {libraries}")
print(f"extra_objects: {extra_objects}")
print(f"extra_link_args: {extra_link_args}")

tokenizer_module = Extension(
    "pyonmttok._ext",
    sources=["pyonmttok/Python.cc"],
    extra_compile_args=cflags,
    extra_link_args=extra_link_args,
    include_dirs=include_dirs,
    library_dirs=library_dirs,
    libraries=libraries,
    extra_objects=extra_objects,
)

setup(
    name="pyonmttok",
    version=_get_project_version(),
    license="MIT",
    description=(
        "Fast and customizable text tokenization library with "
        "BPE and SentencePiece support"
    ),
    long_description=_get_long_description(),
    long_description_content_type="text/markdown",
    author="OpenNMT",
    url="https://opennmt.net",
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3 :: Only",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Topic :: Text Processing :: Linguistic",
        "Topic :: Software Development :: Libraries :: Python Modules",
    ],
    project_urls={
        "Forum": "https://forum.opennmt.net/",
        "Source": "https://github.com/OpenNMT/Tokenizer/",
    },
    keywords="tokenization opennmt unicode bpe sentencepiece subword",
    packages=find_packages(),
    package_data=package_data,
    python_requires=">=3.9",
    setup_requires=["pytest-runner"],
    tests_require=["pytest"],
    ext_modules=[tokenizer_module],
)
