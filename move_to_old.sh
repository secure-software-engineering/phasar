#!/bin/bash

# so wenig wie möglich ändern
# nur cmake!

set -euo pipefail

mkdir -p old

git mv config old/
git mv docs old/
git mv examples old/
git mv img old/
git mv out old/
git mv utils old/
git mv bootstrap.sh old/
git mv .gitmodules old/
git mv .gitlab-ci.yml old/
git mv Brewfile old/
git mv CHANGELOG.md old/
git mv CMakeLists.txt old/
git mv CODE_OF_CONDUCT.md old/
git mv CODING_GUIDELINES.txt old/
git mv Config.cmake.in old/
git mv config.h.in old/
git mv CONTRIBUTING.md old/
git mv *ocker* old/
git mv phasar-clang_more_help.txt old/
git mv phasar-llvm_more_help.txt old/
git mv .dockerignore old/
rm -rf external

git mv githooks/ old/
# git mv old/githooks/ .
git mv .github/workflows/ old/
# git mv old/workflows/ .github/
git mv .pre-commit-config.yaml old/
# git mv old/.pre-commit-config.yaml/ .
