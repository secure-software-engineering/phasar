#!/bin/bash

set -euo pipefail

if [ "$#" -eq 0 ]; then
    echo "[help] run each step source / build / package cached, so you can use trial and error"
    echo "[help] https://docs.conan.io/en/latest/developing_packages/package_dev_flow.html"
    echo "[help]"
    echo "[help] test_conan_package.sh package_to_build/version@ [-o package:option=value ... | -s package:build_type=Release ... (package can refer a dependency)] "
    echo "[help] set version in recipe: version=\"...\""
    exit 0
elif grep -Eqe '^[a-zA-Z]*/[0-9.]*$' <<< "$1"; then
    echo "[error] please provide a \"package/version\""
    exit 1
fi
package="$1"
name="${package//\/*/}"
shift
options=("$@")

(
    cd "$(dirname "$0")"
    test_dir="${name}_test"
    mkdir -p "$test_dir"

    rm -rf "$test_dir/source" &> /dev/null || true
    cmd=(conan source ./"${name}"/ --source-folder="$test_dir/source")
    echo "${cmd[@]}"
    "${cmd[@]}"

    cmd=(conan install ./"${name}"/ "$package" --install-folder="$test_dir/build")
    if [ "${#options[@]}" -gt 0 ]; then
        cmd+=("${options[@]}")
    fi
    echo "${cmd[@]}"
    "${cmd[@]}"

    cmd=(conan build ./"${name}"/ --source-folder="$test_dir/source" --build-folder="$test_dir/build")
    echo "${cmd[@]}"
    "${cmd[@]}"


    cmd=(conan package ./"${name}"/ --source-folder="$test_dir/source" --build-folder="$test_dir/build" --package-folder="$test_dir/package")
    echo "${cmd[@]}"
    "${cmd[@]}"
)
