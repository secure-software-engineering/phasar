# Task: Add LIT/FileCheck Regression Testing (Hybrid with gtest)

## Goal

Add an LLVM-style LIT + FileCheck regression-test suite that drives
`phasar-cli` directly on IR/source fixtures, checking its textual output
(JSON export, dot export, raw results, diagnostics). This complements the
existing gtest suite, which stays as-is for API/library-level unit tests.

## Why

- PhASAR already builds on LLVM/Clang; LIT/FileCheck is the tool the LLVM
  ecosystem uses for exactly this kind of test, so the pattern is familiar
  to contributors.
- Today, a bug reported as "run `phasar-cli` on this `.ll` file, output is
  wrong" gets translated into a `TEST_F` with an inline C++ ground-truth
  map. That's boilerplate-heavy and requires a full unittest rebuild for a
  one-line fixture change.
- With LIT, the same bug becomes a `.ll`/`.c` file with `// RUN:` and
  `// CHECK:` lines next to it. Lower friction for contributors and
  reviewers (CONTRIBUTING.md already asks bug reports to include IR files).

## Non-goals

- Do not migrate existing gtest tests to LIT. gtest keeps testing internal
  APIs and data structures; LIT only tests CLI-observable behavior.
- Do not add fuzzing or property-based testing (separate effort).

## Decision rule (add to CONTRIBUTING.md)

- Bug is only observable/reproducible via `phasar-cli` output (JSON/dot
  export, diagnostics, exit code) -> LIT test.
- Bug is in an internal API/data structure not exposed via the CLI -> gtest.
- When in doubt, prefer LIT for anything that started life as "here is an
  `.ll` file that produces the wrong result."

## Directory layout

```
test/lit/
  lit.cfg.py            # lit configuration (Python)
  lit.site.cfg.py.in     # configured by CMake, generates build-tree lit.site.cfg.py
  README.md              # how to write/run a LIT test
  DataFlow/
    ifds-uninitialized/
      basic.ll
      ...
  ControlFlow/
    icfg-cha/
      ...
  Export/
    json-export/
      ...
```

Mirror the `unittests/PhasarLLVM/...` module layout so contributors can
find the LIT equivalent of a gtest directory by analogy.

## CMake work

1. `find_package` / `find_program(LLVM_LIT ...)`:
   - Prefer `llvm-lit` shipped alongside the LLVM install phasar already
     depends on (same `LLVM_TOOLS_BINARY_DIR` used by `generate_ll_file`
     in `cmake/phasar_macros.cmake`).
   - Fall back to a `lit` found via `find_package(Python3 COMPONENTS
     Interpreter)` + `pip`-installed `lit` package if `llvm-lit` is not
     found. Emit a clear warning and skip the target (do not hard-fail
     the whole build) if neither is available.
2. New `cmake/add_lit_tests.cmake` (or extend `phasar_macros.cmake`) with
   a `add_phasar_lit_testsuite()` function that:
   - Configures `test/lit/lit.site.cfg.py.in` -> build dir, substituting
     `@PHASAR_CLI_PATH@`, `@LLVM_TOOLS_BINARY_DIR@`, `@FILECHECK_PATH@`,
     `@PHASAR_SOURCE_DIR@`.
   - Registers a `check-phasar-lit` custom target running
     `${LLVM_LIT} -sv test/lit` from the build directory, depending on
     `phasar-cli` and `FileCheck` (the latter usually ships with the LLVM
     dev package; `find_program` it the same way `LLVM_COV_PATH` is
     found today).
3. Top-level `check-phasar-tests` (or reuse an existing umbrella target if
   one exists) depends on both `check-phasar-unittests` and
   `check-phasar-lit`, so CI can run one command for everything.
4. Gate the whole feature behind a `PHASAR_BUILD_LIT_TESTS` option
   (default ON, matching the `PHASAR_BUILD_UNITTESTS` precedent in
   `BUILD.md`), so environments without `lit`/`FileCheck` available can
   configure it OFF cleanly instead of failing configure.

## `lit.cfg.py` content

- `config.name = 'PhASAR'`
- `config.suffixes = ['.ll', '.c', '.cpp']`
- `config.test_source_root` = `test/lit`
- `config.test_exec_root` = build-tree equivalent
- Substitutions: `%phasar-cli` -> path to built `phasar-cli`, `%clang`,
  `%clangxx`, `%opt`, `%FileCheck` -> resolved via the same LLVM tool
  search phasar already does in `generate_ll_file` (reuse
  `LLVM_TOOLS_BINARY_DIR` / `PHASAR_LLVM_VERSION`, do not re-implement
  version discovery).
- No special LLVM-style feature flags needed initially (no target
  triples, no shared-lib-only exclusions) -- keep the config minimal and
  add exclusions later only if a real portability issue shows up.

## Fixture format (example)

`test/lit/DataFlow/ifds-uninitialized/basic.c`:

```c
// RUN: %clang -S -emit-llvm -Xclang -disable-O0-optnone %s -o - \
// RUN:   | opt -passes=mem2reg -S \
// RUN:   | %phasar-cli -D ifds-uninit --emit-raw-results - \
// RUN:   | FileCheck %s

int main() {
  int x;
  return x; // CHECK: UndefUse at {{.*}}basic.c:{{[0-9]+}}
}
```

Prefer piping through `%clang`/`opt` inline (as above) over checking in
pre-generated `.ll` files, consistent with how `generate_ll_file` already
regenerates IR at build time rather than committing `.ll` fixtures -- keeps
IR fixtures in sync with the LLVM version actually in use.

## CI work (`.github/workflows/ci.yml`)

- Add `check-phasar-lit` (or the umbrella target) to the existing build
  step for every matrix leg, not just one -- LIT tests are cheap per-test,
  no need to restrict to a single leg the way `run_sample_programs` is
  restricted to `DebugLibdeps`.
- Ensure `FileCheck` is present in the CI image / LLVM install used by the
  matrix (verify for both LLVM 16 and 22.1 legs).
- `DebugCov` leg: confirm `ccov-all` filtering excludes `test/lit/` the
  same way it excludes `external/` and `unittests/` today
  (`CMakeLists.txt:283-303`).

## Migration / first tests

Do not do a big-bang migration. Seed the suite with:

1. 3-5 new tests covering CLI-observable behavior not currently tested at
   all (e.g. JSON/dot export format, CLI error handling on malformed
   input, exit codes).
2. Port the two known IR-dependent/disabled cases surfaced during the
   investigation as good LIT candidates:
   - `unittests/PhasarLLVM/ControlFlow/LLVMBasedICFGExportTest.cpp`
     (`DISABLED_` test referencing issue #741) -- re-express as a LIT test
     checking export output directly, since the original problem was
     IR-dependent output instability, which FileCheck's `{{regex}}`
     patterns handle more gracefully than exact `EXPECT_EQ`.
   - One of the flaky tests currently excluded on Ubuntu > 22 in
     `unittests/CMakeLists.txt:16-27` (e.g. `IDEGeneralizedLCATest`) --
     evaluate whether the flakiness is about exact-match brittleness that
     a FileCheck-based regex check would fix. Do not blindly port all of
     them; some flakiness may be a real solver bug, not a test-format
     issue.

## Documentation

- `test/lit/README.md`: how to write a test, available substitutions, how
  to run just the LIT suite locally (`ninja check-phasar-lit` or direct
  `llvm-lit -sv build/test/lit`).
- `CONTRIBUTING.md`: add the decision rule above; update the "please
  provide unit tests" bug-fix guidance to mention LIT as the default for
  CLI-reproducible bugs.
- `BUILD.md`: document `PHASAR_BUILD_LIT_TESTS` next to
  `PHASAR_BUILD_UNITTESTS`.

## Open questions to resolve during implementation

- Minimum `lit` package version / whether to pin it in a
  `requirements.txt` for reproducibility across CI images.
- Whether `FileCheck` needs to be built from source in any CI leg (LLVM
  dev packages sometimes omit test-only tools) or is reliably present
  wherever `clang`/`opt` already are.
- Whether ARM (`ubuntu-24.04-arm` matrix leg) needs any LIT-specific
  exclusions.

## Rough effort estimate

- CMake/lit plumbing + CI wiring: 1-2 days.
- Seed test suite (5-10 tests) + docs: 1 day.
- Total: ~2-3 days for a working, documented, CI-gated suite; further
  tests added incrementally afterward at low marginal cost.
