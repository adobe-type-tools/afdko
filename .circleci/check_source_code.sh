#!/bin/bash

set -e

# activate virtual environment
source venv/bin/activate
# check Python source files with flake8
flake8 --count --show-source --statistics --config=.flake8

# check C files with cpplint
cpplint --recursive --quiet c/detype1
cpplint --recursive --quiet c/makeotf/makeotf_lib/source
cpplint --recursive --quiet c/makeotf/makeotf_lib/api
cpplint --recursive --quiet c/makeotf/makeotf_lib/resource
cpplint --recursive --quiet c/makeotf/source
cpplint --recursive --quiet c/mergefonts
cpplint --recursive --quiet c/public
cpplint --recursive --quiet c/rotatefont
cpplint --recursive --quiet c/sfntdiff
cpplint --recursive --quiet c/sfntedit
cpplint --recursive --quiet c/spot
cpplint --recursive --quiet c/tx
cpplint --recursive --quiet c/type1
