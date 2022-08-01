#!/bin/bash

set -o errexit
source "$(dirname "$0")/config.sh" # include config
set -o nounset
set -o pipefail

# is conan recipe here? -> build
if [ -f "conanfile.py" ]; then
    conan remove --force "$NAME/$VERSION@$TARGET" || true

    # clean created test builds
    if [ -d "test_package/build" ]; then
        rm -rf "test_package/build" || true
    fi

# else assume its in of the remotes
else
    conan remove --force "$NAME/$VERSION" || true

fi
