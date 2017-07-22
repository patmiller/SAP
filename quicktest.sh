#!/bin/bash
set -e
set -x

if ! python setup.py build > build.log; then
    cat build.log
    exit 1
fi

cd build/lib*
python -m sap.tests
