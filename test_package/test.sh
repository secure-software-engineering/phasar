#!/bin/bash

set -euo pipefail

(
    cd "$(dirname "$0")/.."
    conan create -o "phasar/*:unittests=False" .
)
