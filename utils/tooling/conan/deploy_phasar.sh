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
    options=(-o llvm:shared=False -o llvm:enable_debug=True -o llvm:with_project_clang=True -o llvm:with_project_openmp=True -o llvm:keep_binaries_regex="^(clang|clang\+\+|opt)$")
    cmd=(conan create "$(pwd)/phasar/" "$package" "${options[@]}")
    if [ "$#" -gt 0 ]; then
        user_options+=("$@")
    fi
    
    echo "${cmd[@]}"
    "${cmd[@]}"
)
