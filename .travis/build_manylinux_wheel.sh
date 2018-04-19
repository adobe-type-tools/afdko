#!/bin/bash

# This script is intended to be run from within the official PyPA manylinux1
# Docker container.
# Currently, this was only tested with the 64-bit image:
# 'quay.io/pypa/manylinux1_x86_64'.
#
#   $ docker run --rm -v $PWD:/io $DOCKER_IMAGE /io/.travis/build_manylinux_wheel.sh

set -e -x

# change into root directory of repo
cd /io

# some afdko programs are 32-bit only; these are required to
# build them on a 64-bit linux
yum install -y glibc-devel.i386 libgcc_s.so.1

# For now we only compile wheels for Linux python2.7 64-bit
PYTHON=/opt/python/cp27-cp27mu/bin/python

if [ ! -d ".venv" ]; then
    ${PYTHON} -m pip install --upgrade pip
    ${PYTHON} -m pip install virtualenv
    ${PYTHON} -m virtualenv .venv
fi
source .venv/bin/activate

# build the wheel
pip wheel -v --no-deps .

# repair and save manylinux1 wheel to dist/
for whl in wheelhouse/*.whl; do
    auditwheel repair "$whl" -w /io/dist
done
