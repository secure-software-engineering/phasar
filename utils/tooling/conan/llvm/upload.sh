#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

if [ "$#" -eq "0" ]; then
    echo "[error] please provide a llvm version"
    exit 10    
fi
readonly llvm_version="$1"

(
    cd "$(dirname "$0")"
    bash ../scripts/upload.sh "llvm" "$llvm_version"
)
