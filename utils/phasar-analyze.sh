#!/bin/bash
# phasar-analyze — build a C/C++ project to whole-program LLVM IR (via WLLVM) and
# run a PhASAR analysis on it, end-to-end.
#
# This is the default entry point of the PhASAR Docker image. It wires together
# three steps that are otherwise manual:
#
#   1. build the target project with WLLVM so every translation unit's bitcode is
#      preserved and can be linked into a single whole-program module,
#   2. extract that whole-program module (extract-bc),
#   3. hand the module to phasar-cli with the requested analysis.
#
# The LLVM version used for (1) and (2) MUST match the LLVM version PhASAR was
# linked against (see PHASAR_LLVM_VERSION at build time), otherwise phasar-cli
# will refuse to load the bitcode. In the image that version is baked into
# PHASAR_IR_LLVM_VERSION; override it at runtime with `-e PHASAR_IR_LLVM_VERSION=NN`.

set -euo pipefail

# ---------------------------------------------------------------------------
# escape hatch: `phasar-analyze cli ...` runs the raw phasar-cli unchanged
# ---------------------------------------------------------------------------
if [ "${1:-}" = "cli" ]; then
    shift
    exec phasar-cli "$@"
fi

LLVM_VERSION="${PHASAR_IR_LLVM_VERSION:-22}"
LLVM_BIN_DIR="${PHASAR_LLVM_BIN_DIR:-/usr/lib/llvm-${LLVM_VERSION}/bin}"

# ---------------------------------------------------------------------------
# defaults
# ---------------------------------------------------------------------------
MODE=""            # project | sources | module
PROJECT_DIR=""
BUILD_CMD=""
SOURCES=""
MODULE=""
BINARY=""          # artifact to extract bitcode from (autodetected if empty)
KEEP_GOING="false"
WORKDIR="${PHASAR_WORKDIR:-/tmp/phasar-analyze}"

# forwarded to phasar-cli
ANALYSES=()
ENTRY_POINTS=()
CALL_GRAPH=""
ALIAS_ANALYSIS=""
ANALYSIS_CONFIG=""
OUT_DIR=""
PASSTHROUGH=()     # everything after `--`

usage() {
    cat <<'EOF'
phasar-analyze — build a C/C++ project to whole-program LLVM IR (WLLVM) and run PhASAR.

USAGE:
  phasar-analyze <MODE> [OPTIONS] [-- EXTRA_PHASAR_CLI_ARGS...]
  phasar-analyze cli   <raw phasar-cli args...>     # bypass; call phasar-cli directly

MODE (choose one; defaults to `--project .`):
  --project DIR          Build the project rooted at DIR, then analyze it.
                         Autodetects cmake / ./configure / Makefile.
  --sources "A.c B.cpp"  Compile the given source file(s) directly (no build system)
                         and link them into one module.
  --module FILE.bc|.ll   Skip building; analyze an existing LLVM IR module.

BUILD OPTIONS (project mode):
  --build-cmd "CMD"      Use CMD as the build command instead of autodetection.
                         Run from inside PROJECT_DIR with the WLLVM compilers set.
  --binary PATH          Artifact (executable, .so or .a) to extract bitcode from.
                         Default: autodetect the newest artifact holding bitcode.
  --keep-going           Do not abort if the build returns a non-zero status
                         (analyze whatever bitcode was produced).

ANALYSIS OPTIONS (forwarded to phasar-cli):
  -a, --analysis FLAG    Data-flow analysis, e.g. ifds-uninit, ifds-taint, ide-xtaint,
                         ide-lca, ifds-const. Repeatable. Omit to only build+emit IR.
  -E, --entry NAME       Entry point(s). Default: main. Use __ALL__ for libraries.
  -C, --call-graph ALG   cha | rta | vta | otf | nores  (phasar default: otf)
  -P, --alias-analysis A e.g. union-find, CFLAnders, CFLSteens
      --analysis-config F  Taint/typestate config JSON (sources/sinks/sanitizers).
  -o, --out DIR          Write results into DIR instead of stdout.
  --                     Pass all following args verbatim to phasar-cli
                         (e.g. --emit-pta-as-text, --union-find-aa=ctx-sens).

ENV:
  PHASAR_IR_LLVM_VERSION  LLVM major version for WLLVM (must match PhASAR). Default: 22.
  PHASAR_WORKDIR          Scratch dir for generated bitcode. Default: /tmp/phasar-analyze.

EXAMPLES:
  # Uninitialized-variable analysis on a CMake project mounted at /work:
  phasar-analyze --project /work -a ifds-uninit

  # Double-free taint analysis on two source files:
  phasar-analyze --sources "a.c b.c" -a ide-xtaint \
      --analysis-config /work/double-free-config.json --emit-text-report

  # Points-to as text on an existing module:
  phasar-analyze --module prog.ll -- --emit-pta-as-text \
      --alias-analysis=union-find --union-find-aa=ctx-sens
EOF
}

# ---------------------------------------------------------------------------
# argument parsing
# ---------------------------------------------------------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        --project)          MODE="project"; PROJECT_DIR="$2"; shift 2 ;;
        --sources)          MODE="sources"; SOURCES="$2"; shift 2 ;;
        --module)           MODE="module";  MODULE="$2"; shift 2 ;;
        --build-cmd)        BUILD_CMD="$2"; shift 2 ;;
        --binary)           BINARY="$2"; shift 2 ;;
        --keep-going)       KEEP_GOING="true"; shift ;;
        -a|--analysis)      ANALYSES+=("$2"); shift 2 ;;
        -E|--entry)         ENTRY_POINTS+=("$2"); shift 2 ;;
        -C|--call-graph)    CALL_GRAPH="$2"; shift 2 ;;
        -P|--alias-analysis) ALIAS_ANALYSIS="$2"; shift 2 ;;
        --analysis-config)  ANALYSIS_CONFIG="$2"; shift 2 ;;
        -o|--out)           OUT_DIR="$2"; shift 2 ;;
        -h|--help)          usage; exit 0 ;;
        --)                 shift; PASSTHROUGH=("$@"); break ;;
        *) echo "phasar-analyze: unknown option '$1' (try --help)" >&2; exit 2 ;;
    esac
done

# default mode: analyze the current directory as a project
if [ -z "$MODE" ]; then
    MODE="project"
    PROJECT_DIR="."
fi

# ---------------------------------------------------------------------------
# WLLVM environment — pinned to the LLVM version PhASAR understands
# ---------------------------------------------------------------------------
if [ ! -d "$LLVM_BIN_DIR" ]; then
    echo "phasar-analyze: LLVM toolchain not found at '$LLVM_BIN_DIR'." >&2
    echo "  Set PHASAR_IR_LLVM_VERSION to the version PhASAR was built with," >&2
    echo "  or PHASAR_LLVM_BIN_DIR to the directory holding clang/llvm-link." >&2
    exit 1
fi
export PATH="$LLVM_BIN_DIR:$PATH"
export LLVM_COMPILER="clang"
export LLVM_COMPILER_PATH="$LLVM_BIN_DIR"
# embed debug info so PhASAR's type hierarchy and source-level reporting work
export LLVM_BITCODE_GENERATION_FLAGS="-g ${LLVM_BITCODE_GENERATION_FLAGS:-}"

mkdir -p "$WORKDIR"
WHOLE_PROGRAM_BC="$WORKDIR/whole-program.bc"

# newest file under $1 that carries an embedded WLLVM bitcode section (.llvm_bc).
# A raw grep for the section marker works for executables, shared objects and
# static archives alike, without depending on readelf or a specific tool flag.
find_bc_artifact() {
    local root="$1"
    find "$root" -type f \( -perm -u+x -o -name '*.so' -o -name '*.so.*' -o -name '*.a' \) \
        -printf '%T@ %p\n' 2>/dev/null | sort -nr | while read -r _ path; do
        if LC_ALL=C grep -qa '\.llvm_bc' "$path" 2>/dev/null; then
            echo "$path"; return 0
        fi
    done
}

case "$MODE" in
    # -----------------------------------------------------------------------
    module)
        [ -f "$MODULE" ] || { echo "phasar-analyze: module '$MODULE' not found" >&2; exit 1; }
        WHOLE_PROGRAM_BC="$MODULE"
        ;;

    # -----------------------------------------------------------------------
    sources)
        [ -n "$SOURCES" ] || { echo "phasar-analyze: --sources is empty" >&2; exit 1; }
        echo ">> Compiling sources to LLVM IR with clang-${LLVM_VERSION} ..."
        objs=()
        i=0
        for src in $SOURCES; do
            [ -f "$src" ] || { echo "phasar-analyze: source '$src' not found" >&2; exit 1; }
            case "$src" in
                *.c)             cc="clang" ;;
                *.cc|*.cpp|*.cxx|*.C) cc="clang++" ;;
                *) echo "phasar-analyze: unsupported source '$src'" >&2; exit 1 ;;
            esac
            obj="$WORKDIR/unit_${i}.bc"
            "$cc" -g -O0 -Xclang -disable-O0-optnone -emit-llvm -c "$src" -o "$obj"
            objs+=("$obj")
            i=$((i + 1))
        done
        echo ">> Linking $i module(s) into $WHOLE_PROGRAM_BC ..."
        llvm-link "${objs[@]}" -o "$WHOLE_PROGRAM_BC"
        ;;

    # -----------------------------------------------------------------------
    project)
        [ -d "$PROJECT_DIR" ] || { echo "phasar-analyze: project dir '$PROJECT_DIR' not found" >&2; exit 1; }
        export CC="wllvm"
        export CXX="wllvm++"

        echo ">> Building project in '$PROJECT_DIR' with WLLVM (clang-${LLVM_VERSION}) ..."
        (
            cd "$PROJECT_DIR"
            set +e
            if [ -n "$BUILD_CMD" ]; then
                echo ">> Using custom build command: $BUILD_CMD"
                bash -c "$BUILD_CMD"
            elif [ -f "CMakeLists.txt" ]; then
                echo ">> Detected CMake project"
                cmake -S . -B build-wllvm -DCMAKE_BUILD_TYPE=Debug \
                    -DCMAKE_C_COMPILER=wllvm -DCMAKE_CXX_COMPILER=wllvm++
                cmake --build build-wllvm -j"$(nproc)"
            elif [ -x "configure" ]; then
                echo ">> Detected autotools project"
                ./configure && make -j"$(nproc)"
            elif [ -f "Makefile" ] || [ -f "makefile" ]; then
                echo ">> Detected Makefile"
                make -j"$(nproc)"
            else
                echo "phasar-analyze: could not detect a build system in '$PROJECT_DIR'." >&2
                echo "  Provide one explicitly with --build-cmd \"...\"." >&2
                exit 1
            fi
            rc=$?
            if [ "$rc" -ne 0 ] && [ "$KEEP_GOING" != "true" ]; then
                echo "phasar-analyze: build failed (exit $rc). Use --keep-going to analyze anyway." >&2
                exit "$rc"
            fi
        )

        # locate the artifact holding bitcode
        if [ -z "$BINARY" ]; then
            echo ">> Autodetecting a build artifact that carries bitcode ..."
            BINARY="$(find_bc_artifact "$PROJECT_DIR" || true)"
            [ -n "$BINARY" ] || {
                echo "phasar-analyze: no artifact with embedded bitcode found under '$PROJECT_DIR'." >&2
                echo "  Point at it explicitly with --binary PATH." >&2
                exit 1
            }
            echo ">> Using artifact: $BINARY"
        fi

        echo ">> Extracting whole-program bitcode ..."
        case "$BINARY" in
            *.a) extract-bc -b "$BINARY" -o "$WHOLE_PROGRAM_BC" ;;
            *)   extract-bc "$BINARY" -o "$WHOLE_PROGRAM_BC" ;;
        esac
        ;;
esac

[ -f "$WHOLE_PROGRAM_BC" ] || { echo "phasar-analyze: no bitcode produced at '$WHOLE_PROGRAM_BC'" >&2; exit 1; }
echo ">> Whole-program module ready: $WHOLE_PROGRAM_BC"

# ---------------------------------------------------------------------------
# assemble and run phasar-cli
# ---------------------------------------------------------------------------
cli_args=(-m "$WHOLE_PROGRAM_BC")
for a in "${ANALYSES[@]}"; do cli_args+=(-D "$a"); done
for e in "${ENTRY_POINTS[@]}"; do cli_args+=(-E "$e"); done
[ -n "$CALL_GRAPH" ]      && cli_args+=(-C "$CALL_GRAPH")
[ -n "$ALIAS_ANALYSIS" ]  && cli_args+=(--alias-analysis="$ALIAS_ANALYSIS")
[ -n "$ANALYSIS_CONFIG" ] && cli_args+=(--analysis-config="$ANALYSIS_CONFIG")
[ -n "$OUT_DIR" ]         && cli_args+=(-O "$OUT_DIR")
# if no analysis and no explicit emit was requested, at least emit the IR
if [ "${#ANALYSES[@]}" -eq 0 ] && [ "${#PASSTHROUGH[@]}" -eq 0 ]; then
    cli_args+=(--emit-ir)
fi
[ "${#PASSTHROUGH[@]}" -gt 0 ] && cli_args+=("${PASSTHROUGH[@]}")

echo ">> Running: phasar-cli ${cli_args[*]}"
exec phasar-cli "${cli_args[@]}"
