![PhASAR logo](img/Logo_RGB/Phasar_Logo.png)

PhASAR a LLVM-based Static Analysis Framework
=============================================

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/c944f18c7960488798a0728db9380eb5)](https://app.codacy.com/app/pdschubert/phasar?utm_source=github.com&utm_medium=referral&utm_content=secure-software-engineering/phasar&utm_campaign=Badge_Grade_Dashboard)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/secure-software-engineering/phasar/master/LICENSE.txt)

Version v0619

Secure Software Engineering Group
---------------------------------

+ Philipp Schubert (philipp.schubert@upb.de) and others
+ Please also refer to https://phasar.org/

|branch | status |
| :---: | :---: |
| master | <img src="https://travis-ci.org/secure-software-engineering/phasar.svg?branch=master"> |
| development | <img src="https://travis-ci.org/secure-software-engineering/phasar.svg?branch=development"> |

Currently supported version of LLVM
-----------------------------------
Phasar is currently set up to support LLVM-8.0.0.

What is Phasar?
---------------
Phasar is a LLVM-based static analysis framework written in C++. It allows users
to specify arbitrary data-flow problems which are then solved in a 
fully-automated manner on the specified LLVM IR target code. Computing points-to
information, call-graph(s), etc. is done by the framework, thus you can focus on
what matters.

How do I get started with Phasar?
---------------------------------
We have some documentation on Phasar in our [wiki](https://github.com/secure-software-engineering/phasar/wiki). You probably would like to read 
this README first and then have a look on the material provided on https://phasar.org/
as well. Please also have a look on Phasar's project directory and notice the project directory
examples/ as well as the custom tool tools/myphasartool.cpp.

Building Phasar
---------------
If you cannot work with one of the pre-built versions of Phasar and would like to
compile Phasar yourself, then please check the wiki for installing the 
prerequisites and compilation. It is recommended to compile Phasar yourself in
order to get the full C++ experience and to have full control over the build 
mode.

Please help us to improve Phasar
--------------------------------
You are using Phasar and would like to help us in the future? Then please 
support us by filling out this [web form](https://goo.gl/forms/YG6m3M7sxeUJmKyi1).

By giving us feedback you help to decide in what direction Phasar should stride in
the future and give us clues about our user base. Thank you very much!


Table of Contents
=================

* [Installation](#installation)
    * [Visualization Installation](#visualization-installation)
    * [Brief example using an Ubuntu system](#brief-example-using-an-ubuntu-system)
        * [Installing SQLITE3](#installing-sqlite3)
        * [Installing MySQL](#installing-mysql)
        * [Installing BEAR](#installing-bear)
        * [Installing PYTHON3](#installing-python3)
        * [Installing BOOST](#installing-boost)
        * [Installing LLVM](#installing-llvm)
        * [Installing cURL](#installing-curl)
        * [Installing Node.js](#installing-node.js)
        * [Installing Yarn](#installing-yarn)
        * [Installing MongoDB](#installing-mongodb)
    * [Brief example using a MacOS system](#brief-example-using-a-MacOS-system)
    * [Compile Phasar](#compile-phasar)
        * [A remark on compile time](#a-remark-on-compile-time)
    

Installation
------------
The installation of Phasar is not trivial, since it has some library
dependencies. The libraries needed in order to be able to compile and run
Phasar successfully are the following.

In the following the authors assume that a Unix-like system is used.
Installation guides for the libraries can be found here:

[LLVM / Clang](http://apt.llvm.org/)

[BOOST](http://www.boost.org/doc/libs/1_66_0/more/getting_started/unix-variants.html)

[SQLITE3](https://www.sqlite.org/download.html)

[MySQL](https://www.mysql.com/)

[BEAR](https://github.com/rizsotto/Bear)

[PYTHON](https://www.python.org/)

[ZLIB](https://zlib.net/) - a lossless data-compresion library

[LIBCURSES](http://www.gnu.org/software/ncurses/ncurses.html) - a terminal control library for constructing text user interfaces.

[Doxygen](www.doxygen.org) 

[Graphviz](www.graphviz.org)

### Visualization Installation
To run Phasar from the web interface additional libraries are required and need to be installed **manually**.
Installation guides for the libraries can be found here:

[cURL](https://curl.haxx.se/download.html)

[Node.js](https://nodejs.org/en/download/package-manager/)

[Yarn](https://yarnpkg.com/en/docs/getting-started)

[MongoDB Community Edition](https://docs.mongodb.com/manual/administration/install-community/)

All remaining dependencies are managed by the Yarn dependency manager. Resolved dependencies are stored
in the `yarn.lock` file. To install the dependencies run

`$ yarn install` 

from within the vis/ directory.

### Brief example using an Ubuntu system
In the following we would like to give an complete example of how to install 
Phasar using an Ubuntu (16.04) or Unix-like system. It that case most dependencies
can be installed using the apt package management system.


#### Installing ZLIB
ZLIB can just be installed from the Ubuntu sources:

`$ sudo apt-get install zlib1g-dev`

That's it - done.


#### Installing LIBCURSES
LIBCURSES can just be installed from the Ubuntu sources:

`$ sudo apt-get install libncurses5-dev`

Done!


#### Installing SQLITE3
SQLITE3 can just be installed from the Ubuntu sources:

`$ sudo apt-get install sqlite3 libsqlite3-dev`

That's it - done.

#### Installing MySQL
MySQL can be installed from the Ubuntu sources using:

`$ sudo apt-get install libmysqlcppconn-dev`

#### Installing BEAR
BEAR can just be installed from the Ubuntu sources:

`$ sudo apt-get install bear`

Done!

#### Installing PYTHON3
Python3 can be installed using the Ubuntu sources as well. Just use
the command:

`$ sudo apt-get install python3`

and you are done.


#### Installing DOXYGEN and GRAPHVIZ (Required for generating the documentation)
If you want to generate the documentation running 'make doc' you have to install 
[Doxygen](www.doxygen.org) and [Graphviz](www.graphviz.org).
To install them just use the command:
  
  `$ sudo apt-get install doxygen graphviz`

Done!


#### Installing BOOST
First you have to download the BOOST source files. This can be achieved by:

`$ wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz`

Next the archive file must be extracted by using:

`$ tar xvf boost_1_66_0.tar.gz`

Jump into the directory!

`$ cd boost_1_66_0/`

The next command which prepares the compilation process assumes that you have
write permission in your system's /usr/local/ directory.

`$ ./bootstrap.sh`

If no errors occur boost can now be installed using admin permission:

`$ sudo ./b2 install`

You should be able to find the boost installation on your system now.

`$ ls /usr/local/lib`

Should output a lot of library files prefixed with 'libboost_'.
The result of the command

`$ ls /usr/local/include`

should contain one directory which is called 'boost'. Congratulations, now you
have installed boost.

#### Installing LLVM
When installing LLVM your best bet is probably to install it by using the installer script
install-llvm-*.sh that can be found in Phasar project directory utils/. Parameterize it with the number of cores that
shall be used for compilation (more is better) and tell it where you would like LLVM to
be downloaded and build. E.g. use

`$ ./install-llvm-8.0.0.sh 4 .`

to build llvm-8.0.0 using 4 cores in the current directory.

#### Installing cURL
cURL can be installed from the Ubuntu sources using:

`$ sudo apt-get install libcurl4-openssl-dev curl`

Done!

#### Installing Node.js
To install Node.js 10 on Debian and Ubuntu based Linux distributions, use the following commands:
```
$ curl -sL https://deb.nodesource.com/setup_10.x | sudo -E bash -
$ sudo apt-get install -y nodejs
```

Done!

#### Installing Yarn
Yarn can be installed using their Debian package repository. First, configure the repository:
```
$ curl -sS https://dl.yarnpkg.com/debian/pubkey.gpg | sudo apt-key add -
$ echo "deb https://dl.yarnpkg.com/debian/ stable main" | sudo tee /etc/apt/sources.list.d/yarn.list
```

Then you can simply:

`$ sudo apt-get update && sudo apt-get install yarn`

Done!

#### Installing MongoDB
To install MongoDB using .deb packages, you have to import the public key used by the package management system:

`$ sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 9DA31620334BD75D9DCB49F368818C72E52529D4`

Create a list file for MongoDB (on Ubuntu 16.04):

`$ echo "deb [ arch=amd64,arm64 ] https://repo.mongodb.org/apt/ubuntu xenial/mongodb-org/4.0 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-4.0.list`

Then you can simply:

`$ sudo apt-get update && sudo apt-get install mongodb-org`

Done!

### Brief example using a MacOS system
Mac OS 10.13.1 or higher only !
To install the framework on a Mac we will rely on Homebrew. (https://brew.sh/)

The needed packages are
```
$ brew install boost
$ brew install python3
```

**To be continued.**

### Compile Phasar
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

Navigate into the Phasar directory. The following commands will do the job and compile the Phasar framework:

```
$ mkdir build
$ cd build/
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make -j $(nproc) # or use a different number of cores to compile it
```

Depending on your system, you may get some compiler errors from the json library. If this is the case please change the C++ standard in the top-level CMakeLists.txt:

```
set(CMAKE_CXX_STANDARD 14)
```

After compilation using cmake the following two binaries can be found in the build/ directory:

+ phasar - the actual Phasar command-line tool
+ myphasartool - an example tool that shows how tools can be build on top of Phasar

Use the command:

`$ ./phasar --help`

in order to display the manual and help message.

`$ sudo make install`

Please be careful and check if errors occur during the compilation.

When using CMake to compile Phasar the following optional parameters can be used:

| Parameter : Type|  Effect |
|-----------|--------|
| <b>BUILD_SHARED_LIBS</b> : BOOL | Build shared libraries (default is OFF) |
| <b>CMAKE_BUILD_TYPE</b> : STRING | Build Phasar in 'Debug' or 'Release' mode <br> (default is 'Debug') |
| <b>CMAKE_INSTALL_PREFIX</b> : PATH | Path where Phasar will be installed if <br> “make install” is invoked or the “install” <br> target is built (default is /usr/local) |
| <b>PHASAR_BUILD_DOC</b> : BOOL | Build Phasar documentation (default is OFF) |
| <b>PHASAR_BUILD_UNITTESTS</b> : BOOL | Build Phasar unittests (default is OFF) |
| <b>PHASAR_ENABLE_PAMM</b> : STRING | Enable the performance measurement mechanism <br> ('Off', 'Core' or 'Full', default is Off) |
| <b>PHASAR_ENABLE_PIC</b> : BOOL | Build Position-Independed Code (default is ON) |
| <b>PHASAR_ENABLE_WARNINGS</b> : BOOL | Enable compiler warnings (default is ON) |


#### A remark on compile time
C++'s long compile times are always a pain. As shown in the above, when using cmake the compilation can easily be run in parallel, resulting in shorter compilation times. Make use of it!


#### Running a test solver
To test if everything works as expected please run the following command:

`$ phasar --module test/build_systems_tests/installation_tests/module.ll -D ifds-solvertest`

If you obtain output other than a segmentation fault or an exception terminating the program abnormally everything works as expected.