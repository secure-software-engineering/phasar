#!/bin/bash

set -o errexit
source "$(dirname "$0")/config.sh" "$@" # include config
set -o nounset
set -o pipefail

if ! conan remote list | grep -Eq "^$REMOTE:"; then
    echo "warning remote for $REMOTE not added, so I will only build but not upload"
    echo "execute:"
    echo "conan remote add $REMOTE $REMOTE_URL"
elif ! conan user | grep "'$REMOTE'" | grep -q "[Authenticated]"; then
    echo "conan remote added but you arent authenticated!"
    echo "please create a token with name conan-dev and rights api + api_read"
    echo "https://gitlab.cc-asp.fraunhofer.de/-/profile/personal_access_tokens"
    echo "and execute"
    echo "conan user \"conan-dev\" -r \"$REMOTE\" -p \"\$TOKEN\""
else
    conan upload "$NAME/$VERSION@$TARGET" -r "$REMOTE" --all
fi
