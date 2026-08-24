#!/bin/bash
# install_cmake.sh — install a pinned CMake into a virtualenv.
#
# The distribution's CMake is too old for C++20 modules (Debian 12 ships 3.25,
# module support needs 3.28), so the test image pins its own. PyPI ships CMake as
# a wheel with the binaries inside, and pip verifies the download against the
# hashes passed here.

set -euo pipefail

REQUIREMENTS=""

cleanup() {
    [ -n "$REQUIREMENTS" ] && rm -f "$REQUIREMENTS"
    return 0
}

usage() {
    cat >&2 <<'USAGE'
usage: install_cmake.sh <version> <sha256-wheel> <sha256-sdist> [prefix]

  version        PyPI version of cmake, e.g. 3.28.4
  sha256-wheel   sha256 of the manylinux x86_64 wheel
  sha256-sdist   sha256 of the sdist (pip demands hashes for every candidate)
  prefix         target directory of the venv, default /opt/cmake
USAGE
}

main() {
    if [ "$#" -lt 3 ]; then
        usage
        exit 4
    fi

    local version="$1" sha_wheel="$2" sha_sdist="$3" prefix="${4:-/opt/cmake}"

    REQUIREMENTS="$(mktemp)"
    trap cleanup EXIT
    printf 'cmake==%s \\\n    --hash=sha256:%s \\\n    --hash=sha256:%s\n' \
        "$version" "$sha_wheel" "$sha_sdist" > "$REQUIREMENTS"

    if ! python3 -m venv "$prefix"; then
        printf 'install_cmake.sh: creating a venv at %s failed\n' "$prefix" >&2
        exit 5
    fi
    if ! "$prefix/bin/pip" install --no-cache-dir --require-hashes \
            --only-binary :all: -r "$REQUIREMENTS"; then
        printf 'install_cmake.sh: pip install of cmake==%s failed\n' "$version" >&2
        exit 5
    fi

    verify_installation "$prefix" "$version"
}

# Verifies the exact requested version, not merely that cmake runs.
verify_installation() {
    local prefix="$1" version="$2" reported
    reported="$("$prefix/bin/cmake" --version | sed -n '1s/.*version //p')"
    if [ "$reported" != "$version" ]; then
        printf 'install_cmake.sh: cmake %s installed, %s requested\n' \
            "$reported" "$version" >&2
        exit 5
    fi
}

main "$@"
