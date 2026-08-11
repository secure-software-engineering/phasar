# Analyzing a C/C++ Program with the PhASAR Docker Container

This guide shows how to analyze a whole C/C++ program end-to-end using the PhASAR Docker image. You do **not** need to install LLVM, WLLVM, or PhASAR yourself — the image bundles everything and orchestrates the required steps for you.

## Contents
- [How it works](#how-it-works)
- [Build the image](#build-the-image)
- [Quick start](#quick-start)
- [Providing input](#providing-input)
- [Choosing an analysis](#choosing-an-analysis)
- [Alias analysis & call graph](#alias-analysis--call-graph)
- [Getting results out](#getting-results-out)
- [Taint / typestate configuration](#taint--typestate-configuration)
- [Raw phasar-cli access](#raw-phasar-cli-access)
- [Configuration (environment variables)](#configuration-environment-variables)
- [Troubleshooting](#troubleshooting)

## How it Works

PhASAR analyzes **LLVM IR**, not C/C++ source directly. To analyze a real program it must first be compiled to a single whole-program LLVM module. The image's entry point, `phasar-analyze`, automates the whole pipeline:

```mermaid
flowchart LR
    A[/"your C/C++ project"/] --> B["build with WLLVM (clang)"]
    B --> C(["extract-bc"])
    C --> D[/"whole-program.bc"/]
    D --> E(["phasar-cli"])
    E --> F[/"results"/]

    linkStyle default stroke-width:3px
```

1. **Build with WLLVM** — compiles your project with clang while preserving each translation unit's bitcode (with `-g` for source-level reporting).
2. **`extract-bc`** — links all the preserved bitcode into one whole-program module.
3. **`phasar-cli`** — runs the requested analysis on that module.

All three steps run inside the container; you just provide the program and pick
an analysis.

## Build the image

From the repository root:

```bash
docker build -t phasar .
```

This builds PhASAR against LLVM 22 by default. To use a different LLVM major version, pass build args (the value must match a version PhASAR supports, currently 16–22):

```bash
docker build -t phasar --build-arg llvm_version=16 --build-arg phasar_llvm_version=16 .
```

> [!NOTE]- llvm_version vs phasar_llvm_version
> `phasar_llvm_version` is PhASAR's CMake version string — `22.1` for LLVM 22 (new release scheme), but `16` for LLVM 16.


## Quick Start

Analyze a project mounted at `/work` for uses of uninitialized variables:

```bash
docker run --rm --user "$(id -u):$(id -g)" \
    -v "$PWD:/work" phasar --project /work -a ifds-uninit
```

- `-v "$PWD:/work"` mounts your current directory into the container.
- `--user "$(id -u):$(id -g)"` keeps generated files owned by you, not root.
- `--project /work` autodetects the build system (cmake / `./configure` / make).
- `-a ifds-uninit` selects the analysis.

See runnable, self-contained examples in [`examples/docker-whole-program/`](examples/docker-whole-program/) — run them all with `examples/docker-whole-program/run-all.sh`.

## Providing Input

`phasar-analyze` accepts several input modes — pick one:

| Mode | Use when | Example |
|------|----------|---------|
| `--project DIR` | You have a project with a build system (autodetects cmake / `./configure` / make) | `phasar --project /work -a ifds-uninit` |
| `--build-cmd "CMD"` | The project needs a specific build command | `phasar --project /work --build-cmd "make -j4" -a ifds-uninit` |
| `--sources "A.c B.cpp"` | You just have a few loose source files | `phasar --sources "main.c util.c" -a ifds-uninit` |
| `--binary PATH` | You want to target a specific build artifact (exe, `.so`, `.a`) | `phasar --project /work --binary /work/build/libfoo.a -a ifds-uninit -E __ALL__` |
| `--module FILE.ll\|.bc` | You already have LLVM IR | `phasar --module /work/prog.bc -a ifds-uninit` |

**Entry points.** By default analysis starts at `main`. Use `-E NAME` to set a different entry point, or `-E __ALL__` for code without a `main` (e.g. a library):

```bash
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD:/work" phasar \
    --project /work -a ifds-uninit -E __ALL__
```

In `--project` mode without `--binary`, the wrapper autodetects the newest build artifact that carries embedded bitcode. If detection picks the wrong one, name it with `--binary`.

## Choosing an Analysis

Select a data-flow analysis with `-a` (repeatable). Available analyses:

| Flag (`-a`) | Analysis | Needs `--analysis-config` |
|-------------|----------|:---:|
| `ifds-taint` | Alias-aware taint analysis | ✔ |
| `ide-xtaint` | Taint analysis with limited field-sensitivity | ✔ |
| `ifds-fieldsens-taint` | Field-sensitive taint (CFL) | ✔ |
| `monoifds-taint` | Taint on the MonoIFDS solver | ✔ |
| `sparse-ifds-taint` | Taint on the SparseIFDS solver | ✔ |
| `ifds-uninit` | Uses of uninitialized variables | |
| `ide-lca` | Linear constant propagation | |
| `ide-iia` | Instruction-interaction (what influences what) | |
| `ifds-const` | **EXPERIMENTAL:** Variables actually mutated through the program | |
| `ifds-type` | **EXPERIMENTAL:** Simple type analysis | |
| `ide-stdio-ts` | **EXPERIMENTAL:** libc file-I/O typestate (invalid usages) | |
| `ide-openssl-ts` | **EXPERIMENTAL:** OpenSSL EVP typestate | |
| `inter-mono-taint` | **EXPERIMENTAL:** Taint via inter-procedural Monotone Framework | ✔ |
| `intra-mono-fca` | **EXPERIMENTAL:** Intra-procedural full constant propagation (Monotone) | |

If you omit `-a` (and don't request an emitter), the wrapper just builds the
module and emits its IR (`--emit-ir`), which is handy to confirm the build.

## Alias Analysis & Call Graph

These affect precision/performance of most analyses:

- **Alias analysis** — `-P` / `--alias-analysis`:
  `CFLAnders` (default, legacy), `CFLSteens` (faster), `union-find` (add `--union-find-aa=ctx-sens` or `ctx-ind-sens` after `--`).
- **Call-graph algorithm** — `-C` / `--call-graph`: `otf` (default), `cha`, `rta`, `vta`, `nores`.

```bash
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD:/work" phasar \
    --sources main.c -P CFLSteens -C cha -a ide-xtaint --analysis-config /work/tc.json
```

## Extracting Results

By default results print to **stdout**. You can also emit specific artifacts — pass emitter flags after `--` (everything after `--` goes straight to `phasar-cli`):

| After `--` | Emits |
|------------|-------|
| `--emit-ir` | Preprocessed/annotated IR of the target |
| `--emit-stats` / `--emit-statistics-as-json` | Module statistics |
| `--emit-th-as-text` / `--emit-th-as-dot` / `--emit-th-as-json` | Type hierarchy |
| `--emit-cg-as-dot` / `--emit-cg-as-json` | Call graph |
| `--emit-pta-as-text` / `--emit-pta-as-dot` / `--emit-pta-as-json` | Points-to / alias info |
| `--emit-esg-as-dot` | Exploded super-graph |
| `--emit-raw-results` | Unprocessed solver results |

```bash
# Type hierarchy + call graph, as text/DOT, to stdout:
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD:/work" phasar \
    --sources main.cpp -- --emit-th-as-text --emit-cg-as-dot
```

**Write to files instead of stdout** with `-o DIR` (the directory must exist and be mounted). Results land in a timestamped subdirectory:

```bash
mkdir -p results
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD:/work" -w /work phasar \
    --sources main.cpp -o /work/results -- --emit-cg-as-json --emit-th-as-json
# -> results/<project>-<timestamp>/psr-cg.json, psr-th.json
```

## Taint Configuration

Taint analyses (`ifds-taint`, `ide-xtaint`, `ifds-fieldsens-taint`, `monoifds-taint`, ...) require a JSON config that declares sources, sinks and sanitizers, passed with `--analysis-config`. The config path must be inside the mounted directory. Example (a double-free config — `free`'s argument is both a source and a sink):

```json
{
  "name": "double-free",
  "version": 1.0,
  "functions": [
    { "name": "free", "params": { "source": [0], "sink": [0] } }
  ]
}
```

```bash
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD:/work" -w /work phasar \
    --sources main.c -a ifds-taint --analysis-config double-free-config.json
```

`ret` tags a function's return value (`"ret": "source"`); `params` tags argument indices. Full schema: [`config/TaintConfigSchema.json`](./config/TaintConfigSchema.json).

## Raw phasar-cli Access

`phasar-analyze` is a convenience wrapper. To call `phasar-cli` directly (e.g. on an existing module, or to see all options), use the `cli` escape hatch:

```bash
docker run --rm phasar cli --help
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD:/work" phasar \
    cli -m /work/prog.ll -D ifds-uninit --emit-text-report
```

## Configuration (environment variables)

Set with `docker run -e NAME=VALUE`:

| Variable | Purpose | Default |
|----------|---------|---------|
| `PHASAR_IR_LLVM_VERSION` | LLVM major version WLLVM must emit (must match the version PhASAR was built with) | baked in at build (22) |
| `PHASAR_LLVM_BIN_DIR` | Directory holding the matching `clang`/`llvm-link` | `/usr/lib/llvm-<ver>/bin` |
| `PHASAR_WORKDIR` | Scratch dir for the generated whole-program bitcode | `/tmp/phasar-analyze` |

## Troubleshooting

- **`... does not exist` / module not loaded** — the LLVM version of the bitcode must match the version PhASAR was built with. If you rebuilt the image for a different `llvm_version`, the wrapper picks it up automatically; if you feed a prebuilt `--module`, make sure it was produced by the same LLVM major version.
- **`could not detect a build system`** — pass `--build-cmd "..."` (project mode) or use `--sources`.
- **`no artifact with embedded bitcode found`** — the build produced no linkable bitcode (e.g. it failed). Check the build output, or point at the artifact with `--binary`. For libraries, remember `-E __ALL__`.
- **Files owned by root** — add `--user "$(id -u):$(id -g)"` to `docker run`.
- **Missing source lines in results** — keep debug info (`-g`); the wrapper adds it automatically for `--sources`/`--project` builds.
- **See every phasar-cli option** — `docker run --rm phasar cli --help`, or `docker run --rm phasar --help` for the wrapper's own options.
