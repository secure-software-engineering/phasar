#!/bin/bash
set -euo pipefail

if printf "%s\n" "$@" | grep -Eqe '^--noninteractive|-ni$'; then
    readonly noninteractive="true"
    shift
else
    readonly noninteractive="false"
fi
readonly LLVM_IR_VERSION=15
additional_dependencies=("$@")

(
    source /etc/os-release
    distro="$ID" # ubuntu / debian / alpine / centos / rocky
    codename="${VERSION_CODENAME:-}" # focal / stretch / - / - / -
    distro_version="$VERSION_ID" # 22.04 / 12 / 3.21.2 / 8 / 9.3
    # can be used to adapt to different distros / version

    if "$noninteractive"; then
        export DEBIAN_FRONTEND=noninteractive
    fi

    packages=("${additional_dependencies[@]}")
    packages+=(
        git ca-certificates build-essential cmake ninja-build # build
        "clang-$LLVM_IR_VERSION" # compiler for IR
        "libclang-rt-$LLVM_IR_VERSION-dev" # ASAN
        libz3-dev libssl-dev "libclang-$LLVM_IR_VERSION-dev" "libclang-common-$LLVM_IR_VERSION-dev" # optional build deps
        zlib1g-dev libzstd-dev "llvm-$LLVM_IR_VERSION-dev" # build deps
    )


    pkg_mgr=()
    privileged=()
    if which sudo >/dev/null 2>&1; then
        privileged+=("sudo")
    fi


    if which apt-get >/dev/null 2>&1; then
        pkg_mgr+=("${privileged[@]}" "apt-get")
    else
        echo "Couldn't determine package manager, sry."
        exit 1
    fi

    function check_if_llvm_apt_is_required() {
        # probe if llvm apt repositories are required
        mapfile -t llvm_deps < <(printf "%s\n" "${packages[@]}" | grep -E 'clang-|llvm-')
        mapfile -t llvm_versions < <(printf "%s\n" "${llvm_deps[@]}" | grep -Eo '[0-9]+' | sort | uniq)

        required_versions=()
        for llvm_version in "${llvm_versions[@]}"; do
            mapfile -t current_llvm_deps < <(printf "%s\n" "${llvm_deps[@]}" | grep -E "$llvm_version")
            for dep in "${current_llvm_deps[@]}"; do
                if ! apt search "^$dep$" 2>&1 | grep -qe "$dep"; then
                    echo "warning: couldn't find $dep via apt"
                    required_versions+=("$llvm_version")
                    break
                fi
            done
        done

        if [ "${#required_versions[@]}" -gt 0 ]; then
            if ! "$noninteractive"; then
                echo "It seems I need additional apt repositories for:"
                printf "missing llvm version %s\n" "${required_versions[@]}"
                read -p "Should I add them? (y/n)" choice
            fi
            if "$noninteractive" || echo "$choice" | grep -Eqie '^y|yes$'; then
                "${privileged[@]}" apt-get install -y gnupg ca-certificates
                "${privileged[@]}" apt-key adv -v --fetch-keys https://apt.llvm.org/llvm-snapshot.gpg.key
                for required_version in "${required_versions[@]}"; do
                    echo "deb http://apt.llvm.org/${codename}/ llvm-toolchain-${codename}-$required_version main" | "${privileged[@]}" tee "/etc/apt/sources.list.d/llvm-$required_version-$codename.list"
                done
                "${pkg_mgr[@]}" update
            fi
        fi
    }

    "${pkg_mgr[@]}" update
    check_if_llvm_apt_is_required
    "${pkg_mgr[@]}" install --no-install-recommends -y "${packages[@]}"
    if "$noninteractive"; then
        "${pkg_mgr[@]}" clean
    fi
)
