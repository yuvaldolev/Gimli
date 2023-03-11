#!/bin/bash

set -e

SCRIPT_PATH="$(realpath $0)"
PROJECT_DIRECTORY="$(dirname $SCRIPT_PATH)"

pushd "$PROJECT_DIRECTORY" >/dev/null

cmake -B build -G Ninja .
cmake --build build

popd >/dev/null
