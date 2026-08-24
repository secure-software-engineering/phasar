#!/bin/bash
# install_wllvm.sh — install WLLVM into a virtualenv of its own.
#
# WLLVM lives on PyPI and ships wllvm/wllvm++/extract-bc as console_scripts,
# which pip generates on install. An unpacked wheel does not provide those
# commands, hence pip rather than a direct download. Version and checksums are
# passed as arguments and enforced through --require-hashes.

set -euo pipefail

# File scope, because the EXIT trap runs after main() has returned: a local
# variable would no longer be bound there under `set -u`.
REQUIREMENTS=""

cleanup() {
    [ -n "$REQUIREMENTS" ] && rm -f "$REQUIREMENTS"
    return 0
}

usage() {
    cat >&2 <<'EOF'
usage: install_wllvm.sh <version> <sha256-wheel> <sha256-sdist> [prefix]

  version        PyPI version of wllvm, e.g. 1.3.1
  sha256-wheel   sha256 of the wheel
  sha256-sdist   sha256 of the sdist (pip demands hashes for every candidate)
  prefix         target directory of the venv, default /opt/wllvm
EOF
}

main() {
    if [ "$#" -lt 3 ]; then
        usage
        exit 4
    fi

    local version="$1" sha_wheel="$2" sha_sdist="$3" prefix="${4:-/opt/wllvm}"

    if ! command -v python3 >/dev/null 2>&1; then
        printf 'install_wllvm.sh: python3 not found\n' >&2
        exit 5
    fi

    REQUIREMENTS="$(mktemp)"
    trap cleanup EXIT

    printf 'wllvm==%s \\\n    --hash=sha256:%s \\\n    --hash=sha256:%s\n' \
        "$version" "$sha_wheel" "$sha_sdist" > "$REQUIREMENTS"

    if ! python3 -m venv "$prefix"; then
        printf 'install_wllvm.sh: creating a venv at %s failed\n' "$prefix" >&2
        exit 5
    fi
    if ! "$prefix/bin/pip" install --no-cache-dir --require-hashes \
            --only-binary :all: -r "$REQUIREMENTS"; then
        printf 'install_wllvm.sh: pip install of wllvm==%s failed\n' \
            "$version" >&2
        exit 5
    fi

    verify_installation "$prefix" "$version"
}

# Verifies the exact requested version, not merely that the tools run.
verify_installation() {
    local prefix="$1" version="$2" installed prog

    installed="$("$prefix/bin/pip" show wllvm | sed -n 's/^Version: //p')"
    if [ "$installed" != "$version" ]; then
        printf 'install_wllvm.sh: wllvm %s installed, %s requested\n' \
            "$installed" "$version" >&2
        exit 5
    fi

    for prog in wllvm wllvm++ extract-bc; do
        if [ ! -x "$prefix/bin/$prog" ]; then
            printf 'install_wllvm.sh: %s missing from %s/bin\n' "$prog" "$prefix" >&2
            exit 5
        fi
    done
}

main "$@"
