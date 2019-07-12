#!/bin/bash

set -e

# activate virtual environment
source venv/bin/activate
# check source files
flake8 setup.py
flake8 tests/*.py
pushd python/afdko
flake8 buildcff2vf.py
flake8 buildmasterotfs.py
flake8 checkoutlinesufo.py
flake8 convertfonttocid.py
flake8 fdkutils.py
flake8 makeinstancesufo.py
flake8 makeotf.py
flake8 otf2ttf.py
flake8 otfpdf.py
flake8 pdfmetrics.py
flake8 ttfcomponentizer.py
flake8 ttxn.py
flake8 ufotools.py
popd
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
