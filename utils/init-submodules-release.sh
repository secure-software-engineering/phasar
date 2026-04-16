#!/bin/bash
set -euo pipefail

if (git submodule status 2>&1 || true) | grep -iq "fatal: Not a git repository (or any of the parent directories): .git"; then
  pushd "$(dirname "$0")"/../external/

  git clone --no-checkout https://github.com/nlohmann/json.git
  pushd json/
  git checkout v3.12.0
  popd

  git clone --no-checkout https://github.com/pboettch/json-schema-validator.git
  pushd json-schema-validator/
  git checkout 2.3.0
  popd

  popd
else
  git submodule update --init;
fi
