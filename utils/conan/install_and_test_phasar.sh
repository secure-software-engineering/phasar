#!/bin/bash
#
# deploy the current phasar state into conan using:
# phasar/YYYY.mm.dd+hash
#
# invocation: ./script [conan options only for your build]
set -euo pipefail

(
    cd "$(dirname "$0")"
    calver="$(git show -s --date=format:'%Y.%m.%d' --format='%cd')"
    short_hash="$(git show -s --format='%h')"

    package="phasar/$calver+$short_hash@"
    
    # from conanfile.txt
    cmd=(conan create "$(pwd)/phasar/" --build=missing)
    if [ "$#" -gt 0 ]; then
        cmd+=("$@")
    fi
    
    echo "${cmd[@]}"
    "${cmd[@]}" -s build_type=Release
    echo ""
    echo "testing if install will work after export"
    (
        tmp_dir="$(mktemp -d)"
        cd "$tmp_dir"
        conan install "$package" --build=missing -s build_type=Debug
    )
    echo "deployed $package to your system"
    echo "you maybe want to upload it to your remote via:"
    echo "conan upload -r remoteName $package"
)
