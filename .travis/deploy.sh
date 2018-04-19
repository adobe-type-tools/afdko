#!/bin/bash

# the .travis/before_deploy.sh script must be run first to create
# the virtual environment and build the distribution packages
source .venv/bin/activate
pip install twine

export TWINE_USERNAME=afdko-travis

twine upload dist/*
