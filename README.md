![PhASAR logo](img/Logo_RGB/Phasar_Logo.png)

# PhASAR: A LLVM-based Static Analysis Framework

[![C++ Standard](https://img.shields.io/badge/C++_Standard-C%2B%2B20-blue.svg?style=flat&logo=c%2B%2B)](https://isocpp.org/)
[![GitHub license](https://img.shields.io/badge/license-MIT-blueviolet.svg)](https://raw.githubusercontent.com/secure-software-engineering/phasar/master/LICENSE.txt)
[![GitHub Release](https://img.shields.io/github/v/release/secure-software-engineering/phasar?label=version)](https://github.com/secure-software-engineering/phasar/releases)

## What is PhASAR?

PhASAR is a LLVM-based static analysis framework written in C++.
It allows users to specify arbitrary data-flow problems which are then solved in a fully-automated manner on the specified LLVM IR target code.
Computing points-to information, call-graph(s), etc. is done by the framework, thus you can focus on what matters.

You can find available literature on PhASAR [here](https://github.com/secure-software-engineering/phasar/wiki/Useful-Literature#papers-on-phasar).

### Key Features

- **IFDS/IDE solver**: Interprocedural data-flow solvers based on the IFDS/IDE algorithm
- **WPDS solver (experimental)**: Data-flow solver based on weighted pushdown systems. Can solve any IFDS/IDE problem
- **MonoIFDS solver**: High-performance data-flow solver for *bottom-up* IFDS analyses
- **Sparse analysis**: SparseIFDS/SparseIDE/SparseWPDS for improved performance
- **Call-graph construction**: Several algorithms (CHA, RTA, VTA, alias-based)
- **Type-hierarchy construction**: Extract high-level C++ type information from LLVM IR
- **Points-to/alias infrastructure**: High-performance alias analyses for LLVM IR. Integration with state-of-the-art alias/points-to information from SVF supported
- **Interprocedural CFG (ICFG)**: Connecting control-flow with call-graph information
- **Path-tracking**: Improve results-reporting by reconstruct concrete data-flow paths from IFDS/IDE results
- **Monotone solver**: Simple intra-procedural analysis engine, based on Monotone Frameworks
- **Taint analysis**: Infrastructure for taint-configuration & IFDS/IDE-based taint analysis
- **Modern C++20 API**: Modular, easy-to use interfaces, also for non C++ experts

### How do I get started with PhASAR?

We have some documentation on PhASAR in our [***Wiki***](https://github.com/secure-software-engineering/phasar/wiki). You probably would like to read
this README first.

Please also have a look at PhASAR's project directory, in particular the
[examples](./examples/) directory and the custom tool
`tools/example-tool/myphasartool.cpp`.

You can find PhASAR's API reference [here](https://secure-software-engineering.github.io/phasar/).

## Requirements

### C++ Standard

PhASAR requires at least C++20.

PhASAR supports C++20 modules as an experimental feature.

### LLVM Version

PhASAR supports LLVM versions **between LLVM-16 and LLVM-22.1**, using LLVM-16 by default.
We actively test PhASAR with LLVM-16 and LLVM-22.1, so if something does not work, try these versions instead.
Specify the `PHASAR_LLVM_VERSION` cmake variable to change the LLVM version to use.

## Breaking Changes

To keep PhASAR in a state that is well suited for state-of-the-art research in static analysis, as well as for productive use, we have to make breaking changes. Please refer to [Breaking Changes](./BreakingChanges.md) for detailed information on what was broken recently and how to migrate.

## Building PhASAR

Please refer to [BUILD.md](./BUILD.md) for instructions on how to build PhASAR.

## How to use PhASAR?

The following example shows how to use PhASAR's core concepts of IFDS/IDE analysis, alias analysis, type-hierarchy, call-graph, and taint analysis:

```cpp
#include "phasar.h"

// Load the target LLVM IR
auto IRDB = psr::LLVMProjectIRDB::loadOrExit("target.ll");

// Build alias information, a type-hierarchy, and a taint configuration
// (sources/sinks can come from IR annotations, a JSON file, or callbacks)
psr::LLVMAliasSet AS(&IRDB);
psr::DIBasedTypeHierarchy TH(IRDB);
psr::LLVMTaintConfig TC(IRDB);

// Build the interprocedural CFG using VTA call-graph construction
psr::LLVMBasedICFG ICFG(&IRDB, psr::CallGraphAnalysisType::VTA,
                         {"main"}, &TH, &AS);

// Instantiate and solve the taint analysis
psr::IFDSTaintAnalysis Problem(&IRDB, &AS, &TC, {"main"});
psr::solveIFDSProblem(Problem, ICFG);

// Inspect detected leaks
for (const auto &[Inst, Facts] : Problem.Leaks) {
  llvm::outs() << "Leak at: " << psr::llvmIRToString(Inst) << '\n';
}
```

For more examples, including how to write a custom analysis, see [examples/how-to/](./examples/how-to/).

### Integrating PhASAR into your build

We recommend using PhASAR as a library with `cmake`, using `FetchContent` or as git submodule.

Assuming you have checked out phasar in `external/phasar`, the phasar-related cmake commands may look like this:

```cmake
add_subdirectory(external/phasar EXCLUDE_FROM_ALL)            # Build phasar with your tool

...

target_link_libraries(yourphasartool
    ...
    phasar # Make your tool link against phasar
)
```

Depending on your use of PhASAR you also may need to add LLVM to your build.


For more information please consult our [PhASAR wiki pages](https://github.com/secure-software-engineering/phasar/wiki).

If you have PhASAR *installed*, [Use-PhASAR-as-a-library](https://github.com/secure-software-engineering/phasar/wiki/Using-Phasar-as-a-Library) may be a good start.

### Using PhASAR with Conan v2

To export the recipe and dependencies, execute from the repo root:

- `conan export utils/conan/llvm-core/ --version 15.0.7 --user secure-software-engineering`
- `conan export utils/conan/clang/ --version 15.0.7 --user secure-software-engineering`
- `conan export .`
- View exported: `conan list "phasar/*"`
- [Consume the package](https://docs.conan.io/2/tutorial/consuming_packages.html)

If you just want to use phasar-cli:

- `conan install --tool-requires phasar/... --build=missing -of .`
- `source conanbuild.sh`
- `phasar-cli --help`

## Contributing

You are very welcome to contribute to the PhASAR project.
Just raise an issue or a pull request on GitHub.

For details see [Contributing to PhASAR](https://github.com/secure-software-engineering/phasar/wiki/Contributing-to-PhASAR) and [Coding Conventions](https://github.com/secure-software-engineering/phasar/wiki/Coding-Conventions).

## Secure Software Engineering Group

PhASAR is primarily developed and maintained by the Secure Software Engineering Group at Heinz Nixdorf Institute (University of Paderborn) and Fraunhofer IEM.

PhASAR was initially developed by Philipp Dominik Schubert (@pdschubert)(<philipp.schubert@upb.de>).

Currently, PhASAR is maintained by

- Fabian Schiebel (@fabianbs96)(<fabian.schiebel@uni-paderborn.de>)
- Sriteja Kummita (@sritejakv)
- Lucas Briese (@jusito)
- Martin Mory (@MMory)(<martin.mory@upb.de>)
- *others*
