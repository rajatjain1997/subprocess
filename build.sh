#! /bin/bash

set -eo pipefail

BUILD_HOME=$(greadlink -e $(dirname $0))

cmake -B $BUILD_HOME/_build --preset=dev -DBUILD_SHARED_LIBS=ON
cmake --build $BUILD_HOME/_build -j12
