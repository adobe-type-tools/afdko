#!/bin/bash

# the .travis/before_deploy.sh script has already created a
# virtual environment for the macOS build, in order to build
# the distribution packages. Here we create one only for linux
if [ "$TRAVIS_OS_NAME" == "linux" ]; then
    virtualenv --python=python .venv
fi

source .venv/bin/activate
pip install twine

export TWINE_USERNAME=afdko-travis

twine upload dist/*
