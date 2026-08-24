# Integration tests for the PhASAR toolchain

This is the short guide for adding and running a test case. Each case is a small,
self-contained CMake project plus a declaration of what must hold about the module
the toolchain produces from it. The thing under test is `../PhasarToolchain.cmake`.

## Add a case

Follow this order:

1. Pick the project whose shape you need — `minimal` if the shape does not matter,
   otherwise `static-lib`, `shared-lib`, `multi-exe`, `library-only`, `cxx`,
   `unextractable`, `compiler-id`.
2. Copy `cases-TEMPLATE.case` to `<project>/cases/<name>.case`.
3. Delete what you do not need and declare what must hold. At least one positive
   assertion is required.
4. Run the suite — `PHASAR_TEST_CASE=<substring>` restricts it to your case while
   you iterate. Nothing has to be registered anywhere; cases are discovered.

Then break on purpose what the case claims and check that it goes red. Two of the
original cases passed while asserting a side effect that held either way; only
that step found them.

Minimal case:

```
description             = a shared library is a separate link unit
expect_module_function  = @main
forbid_module_function  = @dynutil_flag
```

A case that needs its own project shape gets a new directory with a
`CMakeLists.txt`, its sources, and a `cases/` beside them.

The field reference is below under [Case fields](#case-fields); the template
carries the same information as comments.

## Running

```bash
docker compose up --attach-dependencies tests
```

Nothing is needed on the host but Docker. `tests` is a gate depending on every
environment, so this one command runs them all and a failure cannot slip past: the
chain stops there and `up` exits non-zero.

`--attach-dependencies` is what shows their output; without it you get an exit code
and nothing to read.

To run a single environment, or to pass extra cmake arguments:

```bash
docker compose run --rm tests-clang22
docker compose run --rm tests-clang22 -DPHASAR_IR_LLVM_VERSION=22
```

Two environments run one after the other, because they share the mounted
workspace:

| Service | Environment |
|---|---|
| `tests-clang22` | clang 22, the newest supported version |
| `tests-clang20` | clang 20 — a host whose clang is not the newest, the shape that let a real bug through twice |
| `tests-clang16` | clang 16, the oldest accepted; skips the module case, which clang 16 cannot build |
| `tests-cmake-floor` | CMake 3.21.4, the lowest accepted; skips the module case, which needs 3.28 |
| `tests` | the gate; every environment is chained ahead of it |

The workspace is bind-mounted, so changes to test projects or the toolchain take
effect without rebuilding, and the produced modules stay readable on the host
under `<project>/build/<case>/phasar-ir/`. They are from whichever environment ran
last.

Ownership needs no configuration: the container starts as root only to read the
owner of the mounted workspace, drops to that user with `setpriv`, and runs
everything as them. A workspace owned by root is refused with an explanation
rather than run.

For extra cmake arguments `up` has no way to pass them through, so use `run`:

```bash
docker compose run --rm tests -DPHASAR_IR_LLVM_VERSION=22
```

Running `./run-all.sh` directly on the host also works and needs `cmake` (>= 3.19),
`clang`, `wllvm`, `file` and `llvm-link`; missing tools are reported together
upfront. On Debian/Ubuntu `llvm-link` lives in `llvm-<N>`, not in `clang-<N>`.

## Case fields

Fields are `key = value`, one per line. Surrounding whitespace is trimmed, the
rest of the value is verbatim; only whole lines starting with `#` are comments. A
key may be repeated to give it several values, so no separator is guessed and no
quoting is needed.

| Field | Repeatable | Meaning |
|---|---|---|
| `description` | | one sentence, shown in the report |
| `cmake_additional_arg` | ✔ | extra cmake argument |
| `cmake_generator` | | default `Ninja` |
| `cmake_configuration` | ✔ | configurations to build, needs a multi-config generator |
| `stubbed_clang` | ✔ | fake compiler as `name=reported-version` |
| `expect_module_function` | ✔ | function the module must define |
| `forbid_module_function` | ✔ | function it must not define |
| `expect_selected_clang` | | compiler the toolchain must choose |
| `expect_message_content` | | substring of the expected diagnostic |
| `expect_message_level` | | `error`, `warning` or `status` |

Placeholders expanded in every value: `@HERE@` (this directory), `@CLANG@` /
`@CLANGXX@` / `@MAJOR@` (the clang found on this host), `@STUBS@` (this case's
stub directory). They exist because hard-wiring `clang-22` once made the suite
fail on a clang-20 host for reasons unrelated to the toolchain.

What the case declares decides what runs: module assertions mean build and
disassemble, a message assertion means configure and inspect the diagnostic, a
selection assertion means read the cache. In every case the exit status is checked
too — `error` must abort, `warning` and `status` must not — and any diagnostic of
our own must carry `What:`, `Why:` and `Fix:`.

### Guards

All case files are validated before the first one runs, and every finding is
reported together. A case file is rejected for an unknown key, a single-valued key
set twice, an empty value, a missing description, `cmake_configuration` without a
multi-config generator, `expect_message_level` and `expect_message_content` given
without each other, and — most importantly — for having no positive assertion at
all. A case with only `forbid_module_function` would pass on an empty module.

### Stubs versus environments

A stub is only for a compiler we cannot install: one that is not available in our
test images, or a binary whose name and reported version disagree. The toolchain
really decides on `clang --version`, so those are only simulable. An installable
host shape belongs in an environment (a second image), not in a stub — that
distinction exists because a stub-only suite hid a real host defect twice.

## Known limitation: C++20 module units carry no bitcode

`modules/` covers what works: CMake runs `clang-scan-deps` (the toolchain has to
supply its path), the project builds, and a module is produced. What does **not**
work is the module unit's own code. wllvm 1.3.1 does not recognise `.cppm` as a
source, so it never stamps the `.llvm_bc` section:

```
main.cpp.o        carries .llvm_bc -> its bitcode is in the module
greeting.cppm.o   does not        -> _ZW8greeting14greeting_valuev is missing
```

The module is therefore incomplete for projects whose module units hold real code.
Facade modules lose nothing — PhASAR's own 30 `*.cppm` files only re-export names.
Adding `cppm|ixx|cxxm|ccm` to wllvm's extension regex was verified to fix it, so
this is a one-line upstream gap rather than a design problem.

The case therefore asserts three things: that configuring **warns** about the
module units, that `@main` is present, and that the module function is *absent*.
The last one is a tripwire, not an endorsement — when it turns red the gap has been
closed, and the case and these docs need updating.

## Layout

Everything here serves testing; the deliverable is one level up in
`../PhasarToolchain.cmake`.

- `run-all.sh` — discovery, validation, assertions
- `cases-TEMPLATE.case` — the case format, copyable
- `Dockerfile`, `compose.yaml` — the two test environments
- `tooling/entrypoint.sh` — adopts the workspace owner, then runs the driver
- `tooling/install_wllvm.sh` — wllvm, pinned by version and hash
- `fixtures/` — the chainload probe
- `minimal/`, `static-lib/`, `shared-lib/`, `multi-exe/`, `library-only/`, `cxx/`,
  `unextractable/`, `compiler-id/`, `launcher/`, `binutils/`, `modules/` — one
  CMake project each, with its `cases/`

`minimal/` is the vehicle for cases that need no particular project shape, which
is why it carries the most cases.

The image's build context is `integration/`, so both the toolchain and `tests/`
end up in it. It builds using the project's own
`utils/InstallAptDependencies.sh`, wired in through `additional_contexts`.
