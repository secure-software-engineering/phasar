![PhASAR logo](img/Logo_RGB/Phasar_Logo.png)

PhASAR a LLVM-based Static Analysis Framework
=============================================

[![C++ Standard](https://img.shields.io/badge/C++_Standard-C%2B%2B17-blue.svg?style=flat&logo=c%2B%2B)](https://isocpp.org/)
[![GitHub license](https://img.shields.io/badge/license-MIT-blueviolet.svg)](https://raw.githubusercontent.com/secure-software-engineering/phasar/master/LICENSE.txt)

Version 0222

Secure Software Engineering Group
---------------------------------

+ Philipp Dominik Schubert (philipp.schubert@upb.de) and others
+ Please also refer to https://phasar.org/

Required version of the C++ standard
------------------------------------
PhASAR requires C++-17.

Currently supported version of LLVM
-----------------------------------
PhASAR is currently set up to support LLVM-13.0.

What is PhASAR?
---------------
PhASAR is a LLVM-based static analysis framework written in C++. It allows users
to specify arbitrary data-flow problems which are then solved in a
fully-automated manner on the specified LLVM IR target code. Computing points-to
information, call-graph(s), etc. is done by the framework, thus you can focus on
what matters.

How do I get started with PhASAR?
---------------------------------
We have some documentation on PhASAR in our [_**wiki**_](https://github.com/secure-software-engineering/phasar/wiki). You probably would like to read
this README first and then have a look on the material provided on https://phasar.org/
as well. Please also have a look on PhASAR's project directory and notice the project directory
examples/ as well as the custom tool tools/myphasartool.cpp.

Building PhASAR
---------------
If you cannot work with one of the pre-built versions of PhASAR and would like to
compile PhASAR yourself, then please check the wiki for installing the
prerequisites and compilation. It is recommended to compile PhASAR yourself in
order to get the full C++ experience and to have full control over the build
mode.

Please help us to improve PhASAR
--------------------------------
You are using PhASAR and would like to help us in the future? Then please
support us by filling out this [web form](https://goo.gl/forms/YG6m3M7sxeUJmKyi1).

By giving us feedback you help to decide in what direction PhASAR should stride in
the future and give us clues about our user base. Thank you very much!


Installation
------------
PhASAR can be installed using the installer scripts as explained in the following.


### Installing PhASAR on an Ubuntu system
In the following, we would like to give an complete example of how to install
PhASAR using an Ubuntu or Unix-like system.

Therefore, we provide an installation script. To install PhASAR, just navigate to the top-level
directory of PhASAR and use the following command:
```
$ sudo ./bootstrap.sh
```

Done!


### Installing PhASAR a MacOS system
Mac OS 10.13.1 or higher only!
To install the framework on a Mac we will rely on Homebrew. (https://brew.sh/)

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
# Go to PhASAR directory run the following commands
$ git submodule init
$ git submodule update
$ mkdir build
$ cd build/
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make -j $(nproc) # or use a different number of cores to compile it
$ sudo make install # if you wish a system-wise installation
```


### Compiling PhASAR (if not already done using the installation scripts)
Set the system's variables for the C and C++ compiler to clang:
```
$ export CC=/usr/local/bin/clang
$ export CXX=/usr/local/bin/clang++
```
You may need to adjust the paths according to your system. When you cloned PhASAR from Github you need to initialize PhASAR's submodules before building it:

```
$ git submodule init
$ git submodule update
```

If you downloaded PhASAR as a compressed release (e.g. .zip or .tar.gz) you can use the `init-submodules-release.sh` script that manually clones the required submodules:

```
$ utils/init-submodules-release.sh
```

Navigate into the PhASAR directory. The following commands will do the job and compile the PhASAR framework:

```
$ mkdir build
$ cd build/
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make -j $(nproc) # or use a different number of cores to compile it
$ sudo make install # if you wish to install PhASAR system wide
```

When you have used the `bootstrap.sh` script to install PhASAR, the above steps are already done.
Use them as a reference if you wish to modify PhASAR and recompile it.

After compilation using cmake the following two binaries can be found in the build/ directory:

+ phasar-llvm - the actual PhASAR command-line tool
+ myphasartool - an example tool that shows how tools can be build on top of PhASAR

Use the command:

`$ ./phasar-llvm --help`

in order to display the manual and help message.

`$ sudo make install`

Please be careful and check if errors occur during the compilation.

When using CMake to compile PhASAR the following optional parameters can be used:

| Parameter : Type|  Effect |
|-----------|--------|
| <b>BUILD_SHARED_LIBS</b> : BOOL | Build shared libraries (default is ON) |
| <b>CMAKE_BUILD_TYPE</b> : STRING | Build PhASAR in 'Debug' or 'Release' mode <br> (default is 'Debug') |
| <b>CMAKE_INSTALL_PREFIX</b> : PATH | Path where PhASAR will be installed if <br> “make install” is invoked or the “install” <br> target is built (default is /usr/local/phasar) |
| <b>PHASAR_BUILD_DOC</b> : BOOL | Build PhASAR documentation (default is OFF) |
| <b>PHASAR_BUILD_UNITTESTS</b> : BOOL | Build PhASAR unit tests (default is ON) |
| <b>PHASAR_BUILD_IR</b> : BOOL | Build PhASAR IR (required for running the unit tests) (default is ON) |
| <b>PHASAR_BUILD_OPENSSL_TS_UNITTESTS</b> : BOOL | Build PhASAR unit tests that require OpenSSL (default is OFF) |
| <b>PHASAR_ENABLE_PAMM</b> : STRING | Enable the performance measurement mechanism <br> ('Off', 'Core' or 'Full', default is Off) |
| <b>PHASAR_ENABLE_PIC</b> : BOOL | Build Position-Independed Code (default is ON) |
| <b>PHASAR_ENABLE_WARNINGS</b> : BOOL | Enable compiler warnings (default is ON) |

You can use these parameters either directly or modify the installer-script `bootstrap.sh`

#### A remark on compile time
C++'s long compile times are always a pain. As shown in the above, when using cmake the compilation can easily be run in parallel, resulting in shorter compilation times. Make use of it!


### Running a test solver
To test if everything works as expected please run the following command:

`$ phasar-llvm --module test/build_systems_tests/installation_tests/module.ll -D ifds-solvertest`

If you obtain output other than a segmentation fault or an exception terminating the program abnormally everything works as expected.

How to use PhASAR?
------------------
Please consult our [PhASAR wiki pages](https://github.com/secure-software-engineering/phasar/wiki).


### Installing PhASAR's Git pre-commit hook
You are very much welcome to contribute to the PhASAR project.
Please make sure that you install our pre-commit hook that ensures your commit adheres to the most important coding rules of the PhASAR project.
For more details please consult [Coding Conventions](https://github.com/secure-software-engineering/phasar/wiki/Coding-Conventions) and [Contributing to PhASAR](https://github.com/secure-software-engineering/phasar/wiki/Contributing-to-PhASAR).

To install the pre-commit hook, please run the following commands in PhASAR's root directory:

```
$ pip install pre-commit
$ pre-commit install

```

Thanks. And have fun with the project.
