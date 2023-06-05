![PhASAR logo](img/Logo_RGB/Phasar_Logo.png)

# PhASAR a LLVM-based Static Analysis Framework

[![C++ Standard](https://img.shields.io/badge/C++_Standard-C%2B%2B17-blue.svg?style=flat&logo=c%2B%2B)](https://isocpp.org/)
[![GitHub license](https://img.shields.io/badge/license-MIT-blueviolet.svg)](https://raw.githubusercontent.com/secure-software-engineering/phasar/master/LICENSE.txt)

Version 0323

## Secure Software Engineering Group

PhASAR is primarily developed and maintained by the Secure Software Engineering Group at Heinz Nixdorf Institute (University of Paderborn) and Fraunhofer IEM.

Lead developers of PhASAR are:  Fabian Schiebel (@fabianbs96)(<fabian.schiebel@iem.fraunhofer.de>), Martin Mory (@MMory)(<martin.mory@upb.de>), Philipp Dominik Schubert (@pdschubert)(<philipp.schubert@upb.de>) and others.

<!--  Please also refer to <https://phasar.org/> -->

## Required version of the C++ standard

PhASAR requires C++-17.

However, building in C++20 mode is supported as an experimental feature. You may enable this by turning the cmake option `PHASAR_EXPERIMENTAL_CXX20` on.
Although phasar currently does not make use of C++20 features (except for some `concept`s behind an #ifdef border), your client application that just *uses* phasar as a library may want to use C++20 ealier.

## Currently supported version of LLVM

PhASAR is currently set up to support LLVM-14.0.*

## What is PhASAR?

PhASAR is a LLVM-based static analysis framework written in C++. It allows users
to specify arbitrary data-flow problems which are then solved in a
fully-automated manner on the specified LLVM IR target code. Computing points-to
information, call-graph(s), etc. is done by the framework, thus you can focus on
what matters.

## Breaking Changes

To keep PhASAR in a state that it is well suited for state-of-the-art research in static analysis, as well as for productive use, we have to make breaking changes. Please refer to [Breaking Changes](./BreakingChanges.md) for detailed information on what was broken recently and how to migrate.

## How do I get started with PhASAR?

We have some documentation on PhASAR in our [***Wiki***](https://github.com/secure-software-engineering/phasar/wiki). You probably would like to read
this README first.
<!-- and then have a look on the material provided on <https://phasar.org/>
as well. -->
Please also have a look on PhASAR's project directory and notice the project directory `examples/` as well as the custom tool `tools/example-tool/myphasartool.cpp`.

## Building PhASAR

It is recommended to compile PhASAR yourself in order to get the full C++ experience and to have full control over the build mode.
However, you may also want to try out one of the pre-built versions of PhASAR or the Docker container.

As a shortcut for the very first PhASAR build on your system, you can use our [bootstrap](./bootstrap.sh) script.
Please note that you must have python installed for the script to work properly.

```bash
./bootstrap.sh
```

Note: If you want to do changes within PhASAR, it is recommended to build it in Debug mode:

```bash
./bootstrap.sh -DCMAKE_BUILD_TYPE=Debug
```

The bootstrap script may ask for superuser permissions (to install the dependencies); however it is not recommended to start the whole script with `sudo`.

For subsequent builds, see [Compiling PhASAR](#compiling-phasar-if-not-already-done-using-the-installation-scripts).

## Please help us to improve PhASAR

You are using PhASAR and would like to help us in the future? Then please
support us by filling out this [web form](https://goo.gl/forms/YG6m3M7sxeUJmKyi1).

By giving us feedback you help to decide in what direction PhASAR should stride in
the future and give us clues about our user base. Thank you very much!

## Installation

PhASAR can be installed using the installer scripts as explained in the following.

### Installing PhASAR on an Ubuntu system

In the following, we would like to give an complete example of how to install
PhASAR using an Ubuntu or Unix-like system.

Therefore, we provide an installation script. To install PhASAR, just navigate to the top-level
directory of PhASAR and use the following command:

```bash
./bootstrap.sh --install
```

The bootstrap script may ask for superuser permissions.

Done!

### Installing PhASAR a MacOS system

Due to unfortunate updates to MacOS and the handling of C++, especially on the newer M1 processors, we can't support native development on Mac.
The easiest solution to develop PhASAR on a Mac right now is to use [dockers development environments](https://docs.docker.com/desktop/dev-environments/). Clone this repository as described in their documentation. Afterwards, you have to login once manually, as a root user by running `docker exec -it -u root <container name> /bin/bash` to complete the rest of the install process as described in this readme (install submodules, run bootstrap.sh, ...).
Now you can just attach your docker container to VS Code or any other IDE, which supports remote development.

### Compiling PhASAR (if not already done using the installation scripts)

Set the system's variables for the C and C++ compiler to clang:

```bash
export CC=/usr/local/bin/clang
export CXX=/usr/local/bin/clang++
```

You may need to adjust the paths according to your system. When you cloned PhASAR from Github you need to initialize PhASAR's submodules before building it:

```bash
git submodule update --init
```

If you downloaded PhASAR as a compressed release (e.g. .zip or .tar.gz) you can use the `init-submodules-release.sh` script that manually clones the required submodules:

```bash
utils/init-submodules-release.sh
```

Navigate into the PhASAR directory. The following commands will do the job and compile the PhASAR framework:

```bash
mkdir build
cd build/
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja -j $(nproc) # or use a different number of cores to compile it
sudo ninja install # only if you wish to install PhASAR system wide
```

When you have used the `bootstrap.sh` script to install PhASAR, the above steps are already done.
Use them as a reference if you wish to modify PhASAR and recompile it.

After compilation using cmake the following two binaries can be found in the build/tools directory:

+ `phasar-cli` - the PhASAR command-line tool (previously called `phasar-llvm`) that provides access to analyses that are already implemented within PhASAR. Use this if you don't want to build an own tool on top of PhASAR.
+ `myphasartool` - an example tool that shows how tools can be build on top of PhASAR

Use the command:

`$ ./phasar-cli --help`

in order to display the manual and help message.

Please be careful and check if errors occur during the compilation.

When using CMake to compile PhASAR the following optional parameters can be used:

| Parameter : Type|  Effect |
|-----------|--------|
| **BUILD_SHARED_LIBS** : BOOL | Build shared libraries -- Not recommended anymore. You may want to use PHASAR_BUILD_DYNLIB instead (default is OFF) |
| **PHASAR_BUILD_DYNLIB** : BOOL | Build one fat shared library (default is OFF) |
| **CMAKE_BUILD_TYPE** : STRING | Build PhASAR in 'Debug', 'RelWithDebInfo' or 'Release' mode (default is 'Debug') |
| **CMAKE_INSTALL_PREFIX** : PATH | Path where PhASAR will be installed if "ninja install” is invoked or the “install” target is built (default is /usr/local/phasar) |
| **PHASAR_CUSTOM_CONFIG_INSTALL_DIR** : PATH | If set, customizes the directory, where configuration files for PhASAR are installed (default is /usr/local/.phasar-config)|
| **PHASAR_ENABLE_DYNAMIC_LOG** : BOOL|Makes it possible to switch the logger on and off at runtime (default is ON)|
| **PHASAR_BUILD_DOC** : BOOL | Build PhASAR documentation (default is OFF) |
| **PHASAR_BUILD_UNITTESTS** : BOOL | Build PhASAR unit tests (default is ON) |
| **PHASAR_BUILD_IR** : BOOL | Build PhASAR IR (required for running the unit tests) (default is ON) |
| **PHASAR_BUILD_OPENSSL_TS_UNITTESTS** : BOOL | Build PhASAR unit tests that require OpenSSL (default is OFF) |
| **PHASAR_ENABLE_PAMM** : STRING | Enable the performance measurement mechanism ('Off', 'Core' or 'Full', default is Off) |
| **PHASAR_ENABLE_PIC** : BOOL | Build Position-Independed Code (default is ON) |
| **PHASAR_ENABLE_WARNINGS** : BOOL | Enable compiler warnings (default is ON) |
| **PHASAR_EXPERIMENTAL_CXX20** : BOOL|Build phasar in C++20 mode. This is an experimental feature  (default is OFF)|

You can use these parameters either directly or modify the installer-script `bootstrap.sh`

#### A remark on compile time

C++'s long compile times are always a pain. As shown in the above, when using cmake the compilation can easily be run in parallel, resulting in shorter compilation times. Make use of it!

### Running a test solver

To test if everything works as expected please run the following command:

`$ phasar-cli -m test/llvm_test_code/basic/module_cpp.ll -D ifds-solvertest`

If you obtain output other than a segmentation fault or an exception terminating the program abnormally everything works as expected.

## How to use PhASAR?

We recomment using phasar as a library with `cmake`.

If you already have installed phasar, [Use-PhASAR-as-a-library](https://github.com/secure-software-engineering/phasar/wiki/Using-Phasar-as-a-Library) may be a good start.

Otherwise, we recommend adding PhASAR as a git submodule to your repository.
In this case, just `add_subdirectory` the phasar submodule directory and add phasar's include folder to your `include_directories` within your `CMakeLists.txt`.

Assuming you have checked out phasar in `external/phasar`, the phasar-related cmake commands may look like this:

```cmake
set(PHASAR_BUILD_UNITTESTS OFF)              # -- Don't build PhASAR's unittests with *your* tool
set(PHASAR_BUILD_IR OFF)                     # --
add_subdirectory(external/phasar)            # Build phasar with your tool
include_directories(external/phasar/include) # To find PhASAR's headers
link_libraries(nlohmann_json::nlohmann:json) # To find the json headers

...

target_link_libraries(yourphasartool
    ...
    phasar # Make your tool link against phasar
)
```

Depending on your use of PhASAR you also may need to add LLVM to your build.

For more information please consult our [PhASAR wiki pages](https://github.com/secure-software-engineering/phasar/wiki).

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
