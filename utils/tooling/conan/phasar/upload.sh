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
    bash ../scripts/upload.sh "phasar" "$phasar_version"
)
