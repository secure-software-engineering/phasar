#!/bin/bash

set -euo pipefail

(
    cd "$(dirname "$0")"
    cd ../..
    conan create . --version dev --build=missing -s build_type=RelWithDebInfo
    cd test_package/build/
    mkdir -p Debug/generators
    cd Debug/generators
    conan install -of . --requires phasar/dev --build=missing -s build_type=Release -g VirtualRunEnv -g CMakeDeps
    conan install -of . --requires phasar/dev --build=missing -s build_type=Debug -g VirtualRunEnv -g CMakeDeps
)
