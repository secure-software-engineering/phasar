Data Flow Analysis in LLVM
==========================

Secure Software Engineering and Data Flow Analysis for C and C++
----------------------------------------------------------------

+ author: Philipp D. Schubert (philipp.schubert@upb.de)


Purpose of this tool
--------------------


Installation
------------
The installation of ourframework is not trivial, since it has some libaray 
dependencies. The libraries needed in order to be able to compile and run
ourframework successfully are the following (it is important that the exact 
versions of these libraries are installed):

+ LLVM / Clang (obivously) version 3.9.1
+ BOOST version 1.63.0 or newer
+ SQLITE3 version 3.11.0 or newer

In the following the authors assume that a unix-like system is used.
Installation guides for the libraries can be found here:

[LLVM / Clang (using apt)](http://apt.llvm.org/)
[BOOST](http://www.boost.org/doc/libs/1_63_0/more/getting_started/unix-variants.html)
[SQLITE3](https://www.sqlite.org/download.html)

It is important that the corresponding include and lib directories are added 
to the search path, such that ourframework is able to find them. Also the 
llvm / clang tools must be added to the search path using the following command:

+ export PATH="/usr/lib/llvm-3.9/bin:$PATH"

This line should be added to the end of your .bashrc as well.

Almost done! After having everything set-up correctly you can now continue the
installtion by compiling ourframework. For the sake of compilation we provide 
two mechanisms:

##### Makefile
Just type make and ourframework will be compiled. As usual 'make clean' will 
delete all compiled and auto-generated files. Using 'make doc' will generate the
doxygen code documentation. The compiled binary can be found in the bin/ 
directory.

##### CMake
If you are a fan of cmake you will probably go this route. The following 
commands will do the job:

$ mkdir build
$ cd build/
$ cmake ..
$ make

After compilation using cmake the binary can be found right in the build 
directory.

If any errors occur during compilation some parts of the library installation 
probably went wrong. Please report detailed error messages to the developers of
ourframework.

After having compiled ourframework runnning a small test example seems adequate.
If errors occur when running the test example your compiler might be 
misconfigured. Please report detailed error messages to the developers of
ourframework.

The test example can be run using the following command:

$ bin/main -module  tests/installation_test.cpp -ifds_uninit

The above command runs a small test example on 'installation_test.cpp'. If 
any errors occure, the program terminates abnormal or a segmentation fault is 
displayed please report detailed error messages to the developers.


Getting started
---------------
## Using an existing analysis


## Writing a static analysis

### Choosing an inter-procedural control-flow graph
The 

### Writing an IFDS analaysis

### Writing an IDE analysis
