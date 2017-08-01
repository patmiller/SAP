#!/bin/bash
set -e
set -x

if ! python2 setup.py build > build.log; then
    cat build.log
    exit 1
fi

cd build/lib*
python2 -m sap.tests
