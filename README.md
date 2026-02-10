![PhASAR logo](img/Logo_RGB/Phasar_Logo.png)

# PhASAR: A LLVM-based Static Analysis Framework

[![C++ Standard](https://img.shields.io/badge/C++_Standard-C%2B%2B20-blue.svg?style=flat&logo=c%2B%2B)](https://isocpp.org/)
[![GitHub license](https://img.shields.io/badge/license-MIT-blueviolet.svg)](https://raw.githubusercontent.com/secure-software-engineering/phasar/master/LICENSE.txt)
[![GitHub Release](https://img.shields.io/github/v/release/secure-software-engineering/phasar?label=version)](https://github.com/secure-software-engineering/phasar/releases)

## What is PhASAR?

PhASAR is a LLVM-based static analysis framework written in C++. It allows users
to specify arbitrary data-flow problems which are then solved in a
fully-automated manner on the specified LLVM IR target code. Computing points-to
information, call-graph(s), etc. is done by the framework, thus you can focus on
what matters.

You can find available literature on PhASAR [here](https://github.com/secure-software-engineering/phasar/wiki/Useful-Literature#papers-on-phasar).

### How do I get started with PhASAR?

We have some documentation on PhASAR in our [***Wiki***](https://github.com/secure-software-engineering/phasar/wiki). You probably would like to read
this README first.
<!-- and then have a look on the material provided on <https://phasar.org/>
as well. -->
Please also have a look on PhASAR's project directory and notice the project directory [examples](./examples/) as well as the custom tool `tools/example-tool/myphasartool.cpp`.

You can find PhASAR's API reference [here](https://secure-software-engineering.github.io/phasar/).


## Secure Software Engineering Group

PhASAR is primarily developed and maintained by the Secure Software Engineering Group at Heinz Nixdorf Institute (University of Paderborn) and Fraunhofer IEM.

PhASAR was initially developed by Philipp Dominik Schubert (@pdschubert)(<philipp.schubert@upb.de>).

Currently, PhASAR is maintained by
- Fabian Schiebel (@fabianbs96)(<fabian.schiebel@uni-paderborn.de>)
- Sriteja Kummita (@sritejakv)
- Lucas Briese (@jusito)
- Martin Mory (@MMory)(<martin.mory@upb.de>)
- *others*

## Required Version of the C++ Standard

**NEW**: PhASAR requires at least C++-20.

PhASAR supports C++20 modules as an experimental feature.

## Currently Supported Version of LLVM

**NEW**: PhASAR is currently set up to support **LLVM-16 and 17**, using LLVM 16 by default.<br>
Specify the `PHASAR_LLVM_VERSION` cmake-variable to change the LLVM version to use.


## Breaking Changes

To keep PhASAR in a state that it is well suited for state-of-the-art research in static analysis, as well as for productive use, we have to make breaking changes. Please refer to [Breaking Changes](./BreakingChanges.md) for detailed information on what was broken recently and how to migrate.

## Building PhASAR

Please refer to [BUILD.md](./BUILD.md) for instructions on how to build PhASAR.

## How to use PhASAR?

We recomment using phasar as a library with `cmake` or `conan`.

If you already have installed phasar, [Use-PhASAR-as-a-library](https://github.com/secure-software-engineering/phasar/wiki/Using-Phasar-as-a-Library) may be a good start.

Otherwise, we recommend adding PhASAR as a git submodule to your repository.
In this case, just `add_subdirectory` the phasar submodule directory within your `CMakeLists.txt`.

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

## How to use with Conan v2 ?

To export the recipe and dependencies execute from repo root:
- `conan export utils/conan/llvm-core/ --version 15.0.7 --user secure-software-engineering`
- `conan export utils/conan/clang/ --version 15.0.7 --user secure-software-engineering`
- `conan export .`
- View exported `conan list "phasar/*"`
- [Consume the package](https://docs.conan.io/2/tutorial/consuming_packages.html)

If you just want to use phasar-cli:
- `conan install --tool-requires phasar/... --build=missing -of .`
- `source conanbuild.sh`
- `phasar-cli --help`

## Please help us to improve PhASAR

You are using PhASAR and would like to help us in the future? Then please
support us by filling out this [web form](https://goo.gl/forms/YG6m3M7sxeUJmKyi1).

By giving us feedback you help to decide in what direction PhASAR should stride in
the future and give us clues about our user base. Thank you very much!

### Installing PhASAR's Git pre-commit hook

You are very much welcome to contribute to the PhASAR project.
Please make sure that you install our pre-commit hook that ensures your commit adheres to the most important coding rules of the PhASAR project.
For more details please consult [Coding Conventions](https://github.com/secure-software-engineering/phasar/wiki/Coding-Conventions) and [Contributing to PhASAR](https://github.com/secure-software-engineering/phasar/wiki/Contributing-to-PhASAR).

To install the pre-commit hook, please run the following commands in PhASAR's root directory:

```bash
pip install pre-commit
pre-commit install
```

Thanks. And have fun with the project.
