#!/bin/bash
#
# Is used to provide non-public conan recipes to a phasar user.
# We currently dont have a private conan registry and some recipes arent in conan-center.
#
# For only llvm this is maybe a bit much, basically its one line "conan export ./llvm llvm/14.0.6@phasar/develop"
# But for arbitrary many dependencies, as long as we dont have an own conan artifactory.
set -euo pipefail

(
    cd "$(dirname "$0")"

    if ! which conan; then
        echo "ERROR you didn't install conan, you can do it with \"pip3 install conan\""
        exit 1
    fi

    export CONAN_LOG_RUN_TO_OUTPUT=True
    export CONAN_LOGGING_LEVEL=debug
    export CONAN_PRINT_RUN_COMMANDS=True

    conan_export() {
        path="$(realpath ../../conanfile.txt)"
        if grep -Eqe "^$1/" "$path"; then
            conan export ./"$1"/ "$(grep --max-count 1 -Fe "$1/" "$path")"
        else
            echo "WARNING $1 not referenced in $path, skipping export because its not needed"
        fi
    }
    
    conan_export llvm
)