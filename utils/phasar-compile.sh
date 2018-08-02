#!/bin/bash

# author: Philipp D. Schubert, Richard Leer

BASEDIR=$(dirname $(readlink -f $0))
cd ${BASEDIR}/..
mkdir -p build/
cd build/
cmake -DPHASAR_BUILD_UNITTESTS=ON ..
make -j $(nproc)
make test
