#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

readonly llvm_version="${1:-14.0.6}"

(
    cd "$(dirname "$0")"
    bash ../scripts/clear.sh "llvm" "$llvm_version"
    bash ../scripts/create.sh "llvm" "$llvm_version" "-o llvm:enable_debug=True -o llvm:shared=False"
)
