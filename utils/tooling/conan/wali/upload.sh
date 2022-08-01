#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

(
    cd "$(dirname "$0")"
    bash ../scripts/upload.sh "wali-opennwa" "2020.07.31"
)
