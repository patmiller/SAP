#!/bin/bash

format_python() {
  if hash autopep8 2>/dev/null; then
    for f in $(find sap/ -name '*.py'); do
      autopep8 --in-place $f
    done
  fi
}

format_python >/dev/null
