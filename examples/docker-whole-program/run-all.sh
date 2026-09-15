#!/bin/bash
# run-all.sh — run every example in this directory through the PhASAR Docker
# image (the `phasar-analyze` end-to-end wrapper). Each example is a tiny,
# self-contained C/C++ program with a known finding.
#
# Usage:
#   ./run-all.sh [IMAGE]      # IMAGE defaults to "phasar"
#
# Build the image first (from the repository root):
#   docker build -t phasar .

set -euo pipefail

IMAGE="${1:-phasar}"
HERE="$(cd "$(dirname "$0")" && pwd)"
# run the container as the current user so generated files aren't owned by root
USER_ARGS=(--user "$(id -u):$(id -g)")

# Each entry: "<dir>|<one-line description>|<phasar-analyze args...>"
# Anything after `--` in the args is forwarded verbatim to phasar-cli.
EXAMPLES=(
  "01-uninitialized-variables|IFDS: use of uninitialized variables (whole-program, 2 files)|--sources 'main.c util.c' -a ifds-uninit"
  "02-linear-constant|IDE: linear constant propagation|--sources main.c -a ide-lca"
  "03-taint-leak|IDE: taint analysis, source->sink leak (custom config)|--sources main.c -a ide-xtaint --analysis-config taint-config.json"
  "04-double-free|IFDS: double-free via taint (free = source & sink)|--sources main.c -a ifds-taint --analysis-config double-free-config.json"
  "05-file-io-typestate|IDE: libc file-I/O typestate (use-after-close)|--sources main.c -a ide-stdio-ts"
  "06-type-hierarchy|Type hierarchy + vtables from C++ (emit)|--sources main.cpp -- --emit-th-as-text"
  "07-call-graph|Call graph incl. virtual dispatch, CHA (emit)|--sources main.cpp -C cha -- --emit-cg-as-dot"
  "08-points-to|Alias/points-to information, CFLAnders (emit)|--sources main.c -- --emit-pta-as-text"
  "09-instruction-interaction|IDE: which instructions influence which|--sources main.c -a ide-iia"
  "10-statistics|LLVM IR statistics of the module (emit)|--sources main.c -- --emit-stats"
  "11-library|Analyze a static library (no main) with all functions as entry points|--project /work -a ifds-uninit -E __ALL__"
)

for entry in "${EXAMPLES[@]}"; do
  IFS='|' read -r dir desc args <<< "$entry"
  echo "############################################################"
  echo "# $dir"
  echo "#   $desc"
  echo "#   phasar-analyze $args"
  echo "############################################################"
  # `eval` so the quoted 'main.c util.c' in args is word-split correctly
  eval docker run --rm "${USER_ARGS[@]}" \
       -v "\"$HERE/$dir:/work\"" -w /work "\"$IMAGE\"" "$args"
  echo
done

echo "All examples finished."
