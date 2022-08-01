#!/bin/bash

set +o errexit

CONANFILE="$(readlink -f "$(pwd)/../../../../conanfile.txt")"
TARGET="intellisectest+intellisectest-application/stable"
REMOTE="itst"
REMOTE_URL="https://gitlab.cc-asp.fraunhofer.de/api/v4/projects/16796/packages/conan"

readonly CONANFILE
#shellcheck disable=SC2034
readonly TARGET
#shellcheck disable=SC2034
readonly REMOTE
#shellcheck disable=SC2034
readonly REMOTE_URL

# name extracted from current working folder
readonly NAME="${1:-"$(pwd | grep -Eo '[^/]+$')"}"
# if no version is provided as first argument extract from conanfile.txt
readonly VERSION="${2:-"$(grep -oP "(?<=$NAME/)[^@]*" "$CONANFILE")"}"

# get additional options for compilation
CONAN_ARGS=()
count=0
for arg in "$@"; do
    count="$((count+1))"
    if [ "$count" -gt "2" ]; then
        CONAN_ARGS+=("$arg")
        echo "[info] additional build configuration with: $arg"
    fi
done

if [ "${#CONAN_ARGS[@]}" == "0" ]; then
    CONAN_ARGS=("")
fi

if [ ! -f "$CONANFILE" ]; then
    echo "[warning] conanfile not existing in $(dirname "$CONANFILE")"
fi

set -o errexit

if [ -z "$NAME" ]; then
    echo "You didn't provide name or I couldn't extract it"
    exit 100
elif [ -z "$VERSION" ]; then
    echo "couldn't read $NAME version from conanfile.txt and you didn't provide it as second argument"
    exit 200
else
    echo "using $NAME/$VERSION"
fi

# set conan log level
#https://docs.conan.io/en/latest/reference/config_files/conan.conf.html
export CONAN_LOG_RUN_TO_OUTPUT=True
export CONAN_LOGGING_LEVEL=debug
export CONAN_PRINT_RUN_COMMANDS=True
