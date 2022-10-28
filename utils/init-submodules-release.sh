#!/bin/bash

source ./safeCommandsSet.sh

if git submodule status 2>&1 | grep -iq "fatal: Not a git repository (or any of the parent directories): .git"; then
safe_cd "$(dirname "$0")"/../external/
git clone git@github.com:google/googletest.git
safe_cd googletest/
git checkout release-1.8.0
safe_cd -
git clone git@github.com:nlohmann/json.git
safe_cd json/
git checkout v3.4.0
safe_cd -
fi
