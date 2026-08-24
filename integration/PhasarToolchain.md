# PhasarToolchain.cmake — reference

Prerequisites and a four-step start are in [`README.md`](README.md). This file
covers everything else: how it works, every option, the diagnostics and the known
limitations.

Extraction is part of the default target, so a plain build yields the module.
Build the `phasar-ir` target on its own to re-extract without relinking.

## How it works

1. The toolchain generates two wrapper scripts (`build-phasar/phasar-ir/clang-<N>`
   and `clang++-<N>`, named after the real drivers so CMake's derivation of the
   LLVM binutils keeps working) and uses them as the compilers. Each exports wllvm's environment
   and execs `wllvm`/`wllvm++`. A compiler launcher would be shorter but fails
   twice over: CMake runs the compiler *without* launchers when identifying it,
   which leaves `CMAKE_<LANG>_COMPILER_ID` empty and breaks
   `target_compile_features`; and the launcher variable belongs to the project,
   which commonly uses it for ccache.
2. wllvm compiles each translation unit with clang as usual, and additionally
   keeps its bitcode, recording the path in an `.llvm_bc` section of the object.
3. `extract-bc` follows those sections after linking and links the bitcode of
   exactly the translation units present in the selected artifact, producing
   `phasar-ir/whole-program.bc`.

Step 3 must run after linking, so the target wiring is deferred to the end of
configuration, once every `add_subdirectory()` has been processed.

## Options

| Option | Default | Meaning |
|---|---|---|
| `CMAKE_C_COMPILER` | auto-selected | the clang to wrap (else `$ENV{CC}`, else searched) |
| `CMAKE_CXX_COMPILER` | auto-selected | used for C++-only projects (else `$ENV{CXX}`) |
| `PHASAR_IR_LLVM_VERSION` | `16` | LLVM major version your `phasar-cli` was built against |
| `PHASAR_IR_BITCODE_FLAGS` | `-g -O0 -Xclang -disable-O0-optnone` | clang flags for the preserved bitcode |
| `PHASAR_IR_CLANG_VERSIONS` | `16;…;22` | accepted clang major versions |
| `PHASAR_IR_WLLVM_BIN_DIR` | autodetected | directory with `wllvm`/`extract-bc` |
| `PHASAR_IR_TARGET` | the sole executable | target to extract from |
| `PHASAR_IR_CHAINLOAD_TOOLCHAIN_FILE` | – | an existing toolchain to load first |

When no compiler is given, all installed clangs are enumerated rather than taking
the first one on `PATH` — several versions usually coexist. Preference order is
the version `phasar-cli` understands, then older accepted versions, then newer
ones; per-version install dirs such as `/usr/lib/llvm-<N>/bin` are searched too.
If nothing matches, the error lists which clangs were found and rejected.

The default `16` matches PhASAR's own default (`PHASAR_LLVM_VERSION`). Set it to
the version *your* `phasar-cli` was built with — otherwise a mismatch surfaces
only when `phasar-cli` refuses the module.

`PHASAR_IR_BITCODE_FLAGS` decides which IR your analyses see, so it is a visible
option rather than a hidden default. `-O0` with `-disable-O0-optnone` keeps the
IR close to the source; `-g -O1 -Xclang -disable-llvm-optzns` is the alternative
when you want cleaned-up IR without optimisation passes.

If both `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER` are set, both must be clang
and of the same major version — otherwise the module would mix bitcode from two
frontends.

`PHASAR_IR_CLANG_VERSIONS` is a policy knob, not a compatibility claim. LLVM
bitcode is backward compatible in principle, so older clang output is usually
readable by a newer PhASAR — verify a pairing before relying on it. Clang newer
than PhASAR's LLVM is warned about and will most likely be rejected.

## Choosing the target

There is one module per build tree, extracted from one artifact. With exactly one
executable it is picked automatically; otherwise name it, because several
executables have several entry points and no common whole-program module:

```bash
cmake -S . -B build-phasar --toolchain <phasar>/integration/PhasarToolchain.cmake \
      -DPHASAR_IR_TARGET=myapp
```

A static or shared library works too (`extract-bc -b` is used for archives).
Library code has no `main`, so analyze it with every function as an entry point:

```bash
phasar-cli -m build-phasar/phasar-ir/whole-program.bc -D ifds-uninit -E __ALL__
```

To analyze several executables, configure one build tree per target.

> [!NOTE]
> A module covers what is **statically linked** into the chosen artifact. Code in
> a `SHARED` library is a separate link unit and is *not* included — extract that
> library as its own target. Static libraries and `add_subdirectory()`
> dependencies are included, since they end up inside the artifact.

## Existing toolchain files

`CMAKE_TOOLCHAIN_FILE` is a single slot. To keep a cross-compilation, embedded or
dependency-manager toolchain, layer it underneath:

```bash
cmake -S . -B build-phasar --toolchain <phasar>/integration/PhasarToolchain.cmake \
      -DPHASAR_IR_CHAINLOAD_TOOLCHAIN_FILE=/path/to/existing-toolchain.cmake
```

Dependencies built outside this build tree (vcpkg ports, Conan packages) do not
carry bitcode and stay opaque to PhASAR. Vendored `add_subdirectory()`
dependencies are covered automatically, since they are part of your build.

## C++20 modules — partially supported

Projects using `*.cppm` configure and build: CMake needs `clang-scan-deps`, looks
for it next to the compiler, finds nothing next to the generated wrapper, so the
toolchain sets `CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS` itself — including the Debian
case where the tool exists only inside `/usr/lib/llvm-<N>/bin`. Needs CMake >= 3.28,
and a clang that CMake's module map supports: clang 20 and 22 build it, clang 16
does not.

> [!WARNING]
> **The code of module units is missing from the module.** wllvm decides what is a
> compilable source with one extension regex that does not list `.cppm`, so those
> objects are never marked and `extract-bc` cannot find their bitcode.

Configuring a target that has module units therefore **warns**, naming the units:
we know what is being left out, so we say it rather than handing over a module that
looks complete.

How much that costs depends on what your module units contain. Facade modules that
only `#include` headers and re-export names hold no code, so nothing is lost;
PhASAR's own 30 `*.cppm` files are of that kind. Module units with real function
bodies are lost entirely.

The cause is a single line in wllvm, and adding `cppm|ixx|cxxm|ccm` to that regex
was verified to fix it completely — module objects then carry their bitcode and the
functions appear in the module. Neither the upstream repository nor the fork that
publishes to PyPI has that change, so using it means patching wllvm yourself.

## Diagnostics

Every message names three things, details last:

```
PhasarToolchain.cmake
What: several executables found (65)
Why: one build tree yields exactly one module, and several entry points have
     no common whole-program module
Fix: pick one with -DPHASAR_IR_TARGET=<target>; analysing several means one
     build tree each
Details: call-graph, example-tool, phasar-cli, ...
```

## Troubleshooting

- **`wllvm not found`** — `pipx install wllvm`, or set `PHASAR_IR_WLLVM_BIN_DIR`.
- **`llvm-link not found (install the llvm-<N> package)`** — installing `clang-<N>`
  alone is not enough; `llvm-link`/`llvm-ar` live in the `llvm-<N>` package.
- **`clang N not in PHASAR_IR_CLANG_VERSIONS`** — select an accepted clang with
  `-DCMAKE_C_COMPILER=`, or widen the list once you have verified that version.
- **`'<path>' is not clang`** — `CMAKE_C_COMPILER`/`CMAKE_CXX_COMPILER` (or
  `CC`/`CXX`) points at a non-LLVM compiler. PhASAR can only analyze LLVM IR.
- **`clang N and clang++ M differ`** — pick one clang version for both languages.
- **`no executable target` / `several executables`** — name one with
  `-DPHASAR_IR_TARGET=`; see above.
- **Module is suspiciously small** — some translation units bypassed the
  wrappers. Check that no target overrides `CMAKE_C_COMPILER`.
- **`several executables`** — expected for large projects; name the one you want
  with `-DPHASAR_IR_TARGET=`. One module per build tree, so analysing several
  binaries means several build trees.
