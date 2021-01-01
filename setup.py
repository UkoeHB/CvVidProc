# -*- coding: utf-8 -*-
# thanks to: https://github.com/pybind/scikit_build_example

from __future__ import print_function

import sys

try:
    from skbuild import setup
except ImportError:
    print(
        "Please update pip, you need pip 10 or greater,\n"
        " or you need to install the PEP 518 requirements in pyproject.toml yourself",
        file=sys.stderr,
    )
    raise

setup(
    name="cvvidproc",
    version="0.4.0",
    description="C++ bindings for multithreaded opencv video processing",
    author="koe",
    license="MIT",
    packages=["cvvidproc"],
    package_dir={"": "PySources"},
    cmake_install_dir="PySources/cvvidproc",
)