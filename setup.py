# -*- coding: utf-8 -*-
# thanks to: https://github.com/pybind/scikit_build_example

from __future__ import print_function

import sys

import os

try:
    from skbuild import setup
except ImportError:
    print(
        "Please update pip, you need pip 10 or greater,\n"
        " or you need to install the PEP 518 requirements in pyproject.toml yourself",
        file=sys.stderr,
    )
    raise

# thanks to https://github.com/EggyLv999/dynet
def get_env():
    # Get environmental variables first
    ENV = dict(os.environ)

    # Get values passed on the command line
    i = 0
    for i, arg in enumerate(sys.argv[1:]):
        try:
            key, value = arg.split("=", 1)
        except ValueError:
            break
        ENV[key] = value 
    del sys.argv[1:i+1]

    return ENV

ENV = get_env()

def extend_cmake_args(args, env_var, arg_type):
    value = ENV.get(env_var)

    if value is not None:
        args.append("-D" + env_var + ":" + arg_type + "=%r" % value)

# cmake args passed in from environment/command line
def get_cmake_args():
    args = []

    for env_var in ("CV_DIR", "CV_INSTALL_DIR"):
        extend_cmake_args(args, env_var, "STRING")

    return args

setup(
    name="cvvidproc",
    version="0.7.5",
    description="C++ bindings for multithreaded opencv video processing",
    author="koe",
    license="MIT",
    packages=["cvvidproc"],
    package_dir={"": "PySources"},
    cmake_install_dir="PySources/cvvidproc",
    cmake_args=get_cmake_args(),
)