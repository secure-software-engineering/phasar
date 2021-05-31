![PhASAR logo](img/Logo_RGB/Phasar_Logo.png)

# PhASAR a LLVM-based Static Analysis Framework

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/c944f18c7960488798a0728db9380eb5)](https://app.codacy.com/app/pdschubert/phasar?utm_source=github.com&utm_medium=referral&utm_content=secure-software-engineering/phasar&utm_campaign=Badge_Grade_Dashboard)
[![C++ Standard](https://img.shields.io/badge/C++_Standard-C%2B%2B17-blue.svg?style=flat&logo=c%2B%2B)](https://isocpp.org/)
[![GitHub license](https://img.shields.io/badge/license-MIT-blueviolet.svg)](https://raw.githubusercontent.com/secure-software-engineering/phasar/master/LICENSE.txt)

Version 0521

## Secure Software Engineering Group

+ Philipp Schubert (philipp.schubert@upb.de) and others
+ Please also refer to https://phasar.org/

|branch | status |
| :---: | :---: |
| master | <img src="https://travis-ci.org/secure-software-engineering/phasar.svg?branch=master"> |
| development | <img src="https://travis-ci.org/secure-software-engineering/phasar.svg?branch=development"> |

## Required version of the C++ standard
Phasar requires C++-17.

## Supported Version(s) of LLVM
Phasar is currently set up to support LLVM-12.0.

## What Is Phasar?
Phasar is an LLVM-based static analysis framework written in C++.
It allows users to specify arbitrary data-flow problems which are then solved in a fully-automated manner on the specified LLVM IR target code.
Computing points-to information, call-graph(s), etc. is done by the framework, thus you can focus on what matters.

## How Do I Get Started With Phasar?
We have some documentation on Phasar in our [_**wiki**_](https://github.com/secure-software-engineering/phasar/wiki).
You probably would like to read this README first and then have a look at the materials provided in our [_**wiki**_](https://github.com/secure-software-engineering/phasar/wiki) and at https://phasar.org/.
Please also have a look at the `examples/` as well as the custom tool `tools/example-tool/myphasartool.cpp`.

## Please Help Us to Improve Phasar
You are using Phasar and would like to help us in the future?
Then please support us by filling out this [web form](https://goo.gl/forms/YG6m3M7sxeUJmKyi1).

By giving us feedback you help to decide in what direction Phasar should stride in the future and give us clues about our user base.
Thank you very much!

## Building and Installing PhASAR
Phasar can be build and installed using the installer script as explained in the following.

### Installing Phasar on an Ubuntu System
To install Phasar, just navigate to the top-level directory of PhASAR and use the following command:
```
$ sudo ./bootstrap.sh
```

Done!

### Installing Phasar on a MacOS System
Mac OS 10.13.1 or higher only!
To install the framework on a Mac we rely on Homebrew (https://brew.sh/).

Please follow the instructions down below.

```
$ brew install boost
$ brew install python3
# Install llvm version 10
$ brew install llvm 
# Setting the paths
# Use LLVM's Clang rather than Apple's Clang compiler
$ export CC=/usr/local/opt/llvm/bin/clang
$ export CXX=/usr/local/opt/llvm/bin/clang++
# Set PATH env variable to /usr/local/opt/llvm/bin
# Go to Phasar directory run the following commands
$ git submodule init
$ git submodule update
$ mkdir build
$ cd build/
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make -j $(nproc) # or use a different number of cores to compile it
$ sudo make install # if you wish a system-wise installation
```

### Installing Phasar on a Windows System

**A solution is under implementation.**

### Compiling Phasar Yourself
Set the system's variables for the C and C++ compiler to clang:
```
$ export CC=/usr/local/bin/clang
$ export CXX=/usr/local/bin/clang++
```
You may need to adjust the paths according to your system.
When you cloned PhASAR from Github you need to initialize PhASAR's submodules before building it:

```
$ git submodule init
$ git submodule update
```

If you downloaded PhASAR as a compressed release (e.g. .zip or .tar.gz) you can use the `utils/init-submodules-release.sh` script that manually clones the required submodules:

```
$ utils/init-submodules-release.sh
```

Navigate to Phasar's root directory.
The following commands will do the job and compile the Phasar framework:

```
$ mkdir build
$ cd build/
$ CC=clang CXX=clang++ cmake -G "Ninja" ..
$ ninja
$ ctest
$ sudo ninja install # for a system wide installation
```

This project supports three build modes `DebugSan`, `Debug`, and `Release`.
By default, the most defensive build `DebugSan` is selected and the unit tests and associated test code are automatically build, too.

When you have used the `bootstrap.sh` script to install Phasar, the above steps are already done.
Use them as a reference if you wish to modify Phasar and recompile it.

After compilation the following two binaries, among others, can be found in the `build/` directory:

+ `build/tools/phasar-llvm/phasar-llvm` - the actual Phasar command-line tool
+ `build/tools/example-tool/myphasartool` - an example tool that shows how custom tools can be build on top of Phasar

Use the command:

`$ ./phasar-llvm --help`

in order to display manual and help message.

Please be careful and check if errors occur during the compilation.

When compiling Phasar the following CMake parameters can be used:

| Parameter : Type | Effect |
|-----------|--------|
| <b>CMAKE_BUILD_TYPE</b> : STRING | Build mode ('DebugSan', 'Debug' or 'Release', default is 'DebugSan') |
| <b>PHASAR_ENABLE_PIC</b> : BOOL | Build position-independed code (default is ON) |
| <b>PHASAR_ENABLE_CLANG_TIDY_DURING_BUILD</b> : BOOL | Enable clang-tidy during build (default is OFF) |
| <b>PHASAR_BUILD_IR</b> : BOOL | Build IR test code (default is ON) |
| <b>PHASAR_BUILD_UNITTESTS</b> : BOOL | Build all tests (default is ON) |
| <b>PHASAR_BUILD_OPENSSL_TS_UNITTESTS</b> : BOOL | Build OPENSSL typestate tests (require OpenSSL, default is OFF) |
| <b>PHASAR_BUILD_DOC</b> : BOOL | Build documentation (default is OFF) |
| <b>PHASAR_DEBUG_LIBDEPS</b> : BOOL | Debug internal library dependencies (private linkage, default is OFF) |
| <b>BUILD_SHARED_LIBS</b> : BOOL | Build shared libraries (default is ON) |
| <b>PHASAR_ENABLE_WARNINGS</b> : BOOL | Enable compiler warnings (default is ON) |
| <b>PHASAR_ENABLE_PIC</b> : BOOL | Build position-independed code (default is ON) |
| <b>PHASAR_ENABLE_PAMM</b> : STRING | Enable the performance measurement mechanism ('Off', 'Core' or 'Full', default is 'Off') |
| <b>PHASAR_ENABLE_DYNAMIC_LOG</b> : BOOL | Allows for switching the logger on or off at runtime (default is ON) |

You can use these parameters either directly or modify the installer-script `bootstrap.sh`

### Code Quality
Use the following command to format the entire code base:
```
$ utils/run-clang-format.py
```

CMake can be set up to automatically run clang-tidy during the build process to indicate potential code smells.
Use the CMake option `-DPHASAR_ENABLE_CLANG_TIDY_DURING_BUILD` to run clang-tidy during the build.
Caution: warnings are treated as errors, here.
Better fix your code ;-)

Use the following command(s) to run clang-tidy separately from the build process on the entire code base:
```
# Assuming you already build the project
$ cd build/
$ run-clang-tidy.py -header-filter='phasar.*'
```

Add the additional `-fix` and `-format` switches to automatically apply fixes suggested by clang-tidy.
```
# Assuming you already build the project
$ cd build/
$ run-clang-tidy.py -header-filter='phasar.*' -fix -format
```

clang-tidy is configured to run slightly less checks on the unit tests as clang-tidy would otherwise complain about the code of the Google Test framework.
Running checks on the external projects that can be found in `external/` is disabled.

## A Remark on Compile Time
C++'s long compile times are always a pain.
When using CMake the compilation can easily be run in parallel resulting in shorter compilation times.
Make use of that!

## Running a Solver Test
To test if everything works as expected, please run the following command:

`$ build/tools/phasar-llvm/phasar-llvm --module test/build_systems_tests/installation_tests/module.ll -D ifds-solvertest`

If you obtain output other than a segmentation fault or an exception terminating the program abnormally everything is fine.

## How to use Phasar?
For more details please consult our [Phasar wiki pages](https://github.com/secure-software-engineering/phasar/wiki).
