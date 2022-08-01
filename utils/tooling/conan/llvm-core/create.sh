#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

(
    cd "$(dirname "$0")"
    bash ../scripts/clear.sh
    bash ../scripts/create.sh "12.0.0" "-o llvm-core:shared=False"
)
