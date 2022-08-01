#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

if [ "$#" -eq "0" ]; then
    echo "[error] please provide a phasar version"
    exit 10    
fi
readonly phasar_version="$1"

(
    cd "$(dirname "$0")"
    bash ../scripts/clear.sh "phasar" "$phasar_version"

    options=()
    for llvm_shared in False; do
    for boost_shared in False; do
        options+=("-o llvm-core:shared=$llvm_shared -o boost:shared=$boost_shared")
    done
    done
    bash ../scripts/create.sh "phasar" "$phasar_version" "${options[@]}"
)
