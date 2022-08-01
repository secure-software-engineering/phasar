#!/bin/bash

set -o errexit

dependencies=("boost" "gtest")

(
    cd "$(dirname "$0")/scripts"
    for dependency in ${dependencies[@]}; do
        bash create.sh "$dependency"
        bash upload.sh "$dependency"
    done
)