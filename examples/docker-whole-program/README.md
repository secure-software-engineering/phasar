# Whole-Program Analysis Examples (Docker)

A set of tiny, self-contained C/C++ programs — each with a known finding — that demonstrate PhASAR's main capabilities **end-to-end through the Docker image**:
Build the program to whole-program LLVM IR with [WLLVM](https://github.com/travitch/whole-program-llvm), then run a PhASAR analysis on it. All of this is handled by the image's
`phasar-analyze` entry point.

## Prerequisites

Build the image once from the repository root:

```bash
docker build -t phasar .
```

The image bundles clang/LLVM (22 by default), WLLVM, and `phasar-cli`, and uses `phasar-analyze` as its entry point. See the top-level `Dockerfile` and `utils/phasar-analyze.sh`.

## Running

Run the whole suite:

```bash
./run-all.sh            # uses image name "phasar"
./run-all.sh my-image   # or a custom image name
```

Run a single example (mount its directory at `/work`):

```bash
docker run --rm --user "$(id -u):$(id -g)" \
    -v "$PWD/01-uninitialized-variables:/work" -w /work \
    phasar --sources "main.c util.c" -a ifds-uninit
```

Running as `--user "$(id -u):$(id -g)"` keeps any generated files owned by you instead of root.

## The examples

| # | Directory | PhASAR feature | Command (args to `phasar-analyze`) |
|---|-----------|----------------|------------------------------------|
| 01 | `01-uninitialized-variables` | **IFDS** data-flow: uninitialized-variable uses (whole-program across 2 files) | `--sources "main.c util.c" -a ifds-uninit` |
| 02 | `02-linear-constant` | **IDE** data-flow: linear constant propagation | `--sources main.c -a ide-lca` |
| 03 | `03-taint-leak` | **Taint** (IDE): source → sink leak with a custom config | `--sources main.c -a ide-xtaint --analysis-config taint-config.json` |
| 04 | `04-double-free` | **Taint** (IFDS): double-free (`free` tagged as source *and* sink) | `--sources main.c -a ifds-taint --analysis-config double-free-config.json` |
| 05 | `05-file-io-typestate` | **Typestate** (IDE): libc file-I/O use-after-close | `--sources main.c -a ide-stdio-ts` |
| 06 | `06-type-hierarchy` | **Type hierarchy** + vtables reconstructed from C++ | `--sources main.cpp -- --emit-th-as-text` |
| 07 | `07-call-graph` | **Call graph** incl. virtual dispatch (CHA) | `--sources main.cpp -C cha -- --emit-cg-as-dot` |
| 08 | `08-points-to` | **Alias / points-to** information (CFLAnders) | `--sources main.c -- --emit-pta-as-text` |
| 09 | `09-instruction-interaction` | **IDE** data-flow: instruction-interaction analysis | `--sources main.c -a ide-iia` |
| 10 | `10-statistics` | **LLVM IR statistics** of the module | `--sources main.c -- --emit-stats` |
| 11 | `11-library` | **Library** (no `main`) analyzed with **all** functions as entry points | `--project /work -a ifds-uninit -E __ALL__` |

Anything after `--` is forwarded verbatim to `phasar-cli` (used above for the `--emit-*` reporting flags).

## Ways to Provide Input

The examples above use `--sources` and `--project`, but `phasar-analyze` accepts several input modes (see `phasar-analyze --help`).
We support all of the following:

| Mode | When to use | Example |
|------|-------------|---------|
| `--sources "a.c b.cpp"` | A few loose source files | `phasar --sources "main.c util.c" -a ifds-uninit` |
| `--project DIR` | A real project; autodetects cmake / `./configure` / make | `phasar --project /work -a ifds-uninit` |
| `--build-cmd "CMD"` | A project with a non-standard build | `phasar --project /work --build-cmd "make -j4" -a ifds-uninit` |
| `--binary PATH` | Point at a specific build artifact (exe, `.so`, `.a`) | `phasar --project /work --binary /work/build/libfoo.a -a ifds-uninit -E __ALL__` |
| `--module FILE.ll\|.bc` | You already have LLVM IR | `phasar --module /work/prog.bc -a ifds-uninit` |

For code without a `main` (libraries), pass `-E __ALL__` to use every function definition as an entry point.

### Writing Results to Files

By default results go to stdout. Pass `-o DIR` (mount it, and it must exist) to write results into a timestamped subdirectory instead — useful with the JSON emitters for machine-readable output:

```bash
mkdir -p results
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD:/work" -w /work phasar \
    --sources main.cpp -o /work/results -- --emit-cg-as-json --emit-th-as-json
# -> results/<project>-<timestamp>/psr-cg.json, psr-th.json
```

## Beyond these Examples

`phasar-analyze` is a thin wrapper over `phasar-cli`; the examples above are a representative slice, not the full set. To see everything available:

```bash
docker run --rm phasar cli --help
```

Some notable options not exercised here:

- **More data-flow analyses** (`-a`/`-D`): `ifds-taint`, `ifds-fieldsens-taint`,  `monoifds-taint`, `sparse-ifds-taint`, `inter-mono-taint`, `ifds-const`, `ifds-type`, `ide-openssl-ts`, `ide-fiia`, `intra-mono-fca`, ... . These cover the different solver families: IFDS, IDE, MonoIFDS, SparseIFDS, and intra-/inter-procedural Monotone Frameworks.
- **Alias analyses** (`--alias-analysis`): `CFLAnders` (default, legacy), `CFLSteens` (faster), `union-find` (with `--union-find-aa=ctx-sens` / `ctx-ind-sens`).
- **Call-graph algorithms** (`-C`): `cha`, `rta`, `vta`, `otf` (default), `nores`.
- **Emitters**: `--emit-ir`, `--emit-cg-as-{dot,json}`, `--emit-th-as-{text,dot,json}`, `--emit-pta-as-{text,dot,json}`, `--emit-statistics-as-json`, `--emit-esg-as-dot`, `--emit-raw-results`.
- **Results to files** instead of stdout: add `-o <dir>` (mount it, too).

## Notes

- The wrapper compiles with `-g`, so findings carry **source-level** file/line information (see the `ifds-uninit` output). Keep debug info in your own builds for the same benefit.
- For real, multi-file projects use `--project <dir>` (autodetects cmake/`./configure`/make and builds with WLLVM) or `--build-cmd "..."` instead of `--sources`. See `phasar-analyze --help`.
- Taint/typestate configs use PhASAR's taint-config JSON (see `config/TaintConfigSchema.json`).
