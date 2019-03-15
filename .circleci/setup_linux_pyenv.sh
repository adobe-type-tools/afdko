#!/bin/bash

# create virtual environment
$PYTHON -m venv venv
# activate virtual environment
source venv/bin/activate
# install and update dependencies
$PIP install -U -q pip setuptools wheel flake8 cpplint
$PIP install cibuildwheel
# print versions
$PYTHON --version
$PIP --version
$PIP list
