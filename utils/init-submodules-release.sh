#!/bin/bash

source ./safeCommandsSet.sh

if git submodule status 2>&1 | grep -iq "fatal: Not a git repository (or any of the parent directories): .git"; then

safe_cd "$(dirname "$0")"/../external/

git clone --no-checkout https://github.com/nlohmann/json.git
safe_cd json/
git checkout v3.11.3
safe_cd -

git clone --no-checkout https://github.com/pboettch/json-schema-validator.git
safe_cd json/
git checkout 2.3.0
safe_cd -

else git submodule update --init;
fi
