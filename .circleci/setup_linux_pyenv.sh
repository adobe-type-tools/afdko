#!/bin/bash

set -e

# create virtual environment
$PYTHON -m venv venv
# activate virtual environment
source venv/bin/activate
# install and update dependencies
$PIP install --disable-pip-version-check -U -q pip setuptools wheel flake8 cpplint
$PIP install pytest-cov codecov
# print versions
$PYTHON --version
$PIP --version
$PIP list
