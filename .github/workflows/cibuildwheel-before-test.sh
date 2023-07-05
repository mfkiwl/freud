#!/bin/bash
if [ -z $1 ]; then
    echo "A package directory must be provided as the first argument."
    exit 1
fi

PACKAGE_DIR=$1

if [[ $(python --version 2>&1) == *"3.7."* ]]; then
  # Python 3.7 is only supported with oldest requirements
  pip install -U -r "${PACKAGE_DIR}/.github/workflows/oldest-test-reqs.txt" --progress-bar=off
elif [[ $(python --version 2>&1) == *"3.8."* ]]; then
  # Python 3.8 needs compatible requirements
  pip install -U -r "${PACKAGE_DIR}/requirements/requirements-test-compatible.txt" --progress-bar=off
else
  pip install -U -r "${PACKAGE_DIR}/requirements/requirements-test.txt" --progress-bar=off
fi

# Allow parallel tests to speed up CI
pip install -U pytest-xdist --progress-bar=off
