#!/bin/bash

# create virtual environment
$PYTHON -m venv venv
# activate virtual environment
source venv/bin/activate
# install and update dependencies
$PIP install -U -q pip setuptools wheel
$PIP install git+https://github.com/adobe-type-tools/cibuildwheel
# print versions
$PYTHON --version
$PIP --version
$PIP list
