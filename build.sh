#! /bin/bash

set -eo pipefail

BUILD_HOME=$(greadlink -e $(dirname $0))

cmake -B $BUILD_HOME/_build -S $BUILD_HOME
cd $BUILD_HOME/_build && make