#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

(
    cd "$(dirname "$0")"
    bash ../scripts/clear.sh "wali-opennwa" "2020.07.31"
    for llvm_shared in True; do
        bash ../scripts/create.sh "wali-opennwa" "2020.07.31" "-o llvm-core:shared=$llvm_shared"
    done
)
