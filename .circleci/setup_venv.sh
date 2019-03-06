#!/bin/bash

# create virtual environment
python -m venv venv
# activate virtual environment
source venv/bin/activate
# install and update dependencies
pip install -U -q pip setuptools wheel flake8 cpplint
pip install git+https://github.com/adobe-type-tools/cibuildwheel
# print versions
python --version
pip --version
pip list
