#!/bin/bash
set -e

source ./utils/safeCommandsSet.sh

echo "Getting the complete Build EAR source code"
tmp_dir="$(mktemp -d "Bear.XXXXXXXX" --tmpdir)"
git clone https://github.com/rizsotto/Bear.git "${tmp_dir}"
mkdir -p "${tmp_dir}"/build
safe_cd "${tmp_dir}"/build
cmake ..
make install
make check
make package
rm -rf "${tmp_dir}"
