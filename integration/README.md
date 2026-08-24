# Analyzing your CMake project with PhASAR

PhASAR analyzes LLVM IR, not C or C++ source. `PhasarToolchain.cmake` configures a
**separate** build tree that compiles your project through WLLVM and reassembles a
whole-program LLVM module from it; your normal build tree stays untouched.

## Preconditions

| | |
|---|---|
| Your code compiles with clang | No tooling can remove this: LLVM IR only comes from an LLVM frontend. GCC-only or MSVC-only code has to be fixed first. |
| CMake | >= 3.21 (`--toolchain`). C++20 modules additionally need >= 3.28. |
| clang | 16 to 22. Should match the LLVM version your `phasar-cli` was built with; a newer clang is warned about and will most likely be rejected. |
| wllvm | `pipx install wllvm` — provides `wllvm`, `wllvm++`, `extract-bc` |
| `file` | wllvm shells out to it; without it the bitcode is written but never marked |
| `llvm-link`, `llvm-ar` | used by `extract-bc`; on Debian/Ubuntu in `llvm-<N>`, **not** in `clang-<N>` |
| `clang-scan-deps` | only for C++20 modules; on Debian in `clang-tools-<N>` |
| Host | POSIX. wllvm relies on ELF section stamping. |

Missing tools are reported when you configure, with the package to install.

## Getting started

1. Install the prerequisites above.
2. Configure a build tree dedicated to analysis:

   ```bash
   cmake -S . -B build-phasar \
         --toolchain <phasar>/integration/PhasarToolchain.cmake \
         -DPHASAR_IR_LLVM_VERSION=<your phasar-cli's LLVM major version>
   ```

3. Build it. Extraction is part of the default target:

   ```bash
   cmake --build build-phasar
   ```

4. Analyze the module:

   ```bash
   phasar-cli -m build-phasar/phasar-ir/whole-program.bc -D ifds-uninit
   ```

If your project has more than one executable, step 2 stops and asks you to pick
one with `-DPHASAR_IR_TARGET=<target>` — one module per build tree, so analyzing
several binaries means one build tree each. A library works as a target too;
analyze it with `phasar-cli … -E __ALL__`.

## What you get, and what you do not

The module contains everything **statically linked** into the chosen artifact,
including static libraries and `add_subdirectory()` dependencies. It does **not**
contain code from shared libraries (extract those as their own target) or from
C++20 module units — wllvm does not recognise `*.cppm`. Configuring a target that
has module units **warns** and names them, so this is not silent; see
[`PhasarToolchain.md`](PhasarToolchain.md) for what it costs and the one-line fix.

## Where to look next

| | |
|---|---|
| [`PhasarToolchain.md`](PhasarToolchain.md) | all options, diagnostics, troubleshooting, limitations |
| [`tests/README.md`](tests/README.md) | the test suite — only needed if you change the toolchain |
| `PhasarToolchain.cmake` | the deliverable; it is the only file you need |
