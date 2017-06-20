Data Flow Analysis in LLVM
==========================

Secure Software Engineering - Data Flow Analysis for C and C++
----------------------------------------------------------------

+ author: Philipp D. Schubert (philipp.schubert@upb.de)


Purpose of this tool
--------------------

### Errors
This framework is still under heavy development. Thus, it might contain errors that
are (un)known to the developers. If you find an error please send mail and report
it to the developers. The report should include at least a summary of what you were doing when you hit the error and a complete error message (if possible). We will try to fix bugs as
quickly as possible, please help us achieving this goal.

Installation
------------
The installation of our framework is not that trivial, since it has some library 
dependencies. The libraries needed in order to be able to compile and run
our framework successfully are the following (it is important that the exact 
versions of these libraries are installed if not stated otherwise):

+ LLVM / Clang version 3.9.1
+ BOOST version 1.63.0 or newer
+ SQLITE3 version 3.11.0 or newer
+ BEAR bear 2.2.0 or newer
+ PYTHON 3.x

In the following the authors assume that a unix-like system is used.
Installation guides for the libraries can be found here:

[LLVM / Clang (using apt)](http://apt.llvm.org/)

[BOOST](http://www.boost.org/doc/libs/1_64_0/more/getting_started/unix-variants.html)

[SQLITE3](https://www.sqlite.org/download.html)

[BEAR](https://github.com/rizsotto/Bear)

[PYTHON](https://www.python.org/)

### Brief example using an Ubuntu system
In the following we would like to give an complete example of how to install 
our framework using an Ubuntu (16.04) or Unix-like system.


#### Installing ZLIB
ZLIB can just be installed from the Ubuntu sources:

$ sudo apt-get install zlib1g-dev

That's it - done.

[ZLIB](https://zlib.net/) is a lossless data-compresion library.


#### Installing LIBCURSES
LIBCURSES can just be installed from the Ubuntu sources:

$ sudo apt-get install libncurses5-dev

[LIBCURSES](http://www.gnu.org/software/ncurses/ncurses.html) is terminal control library for constructing text user interfaces.


#### Installing SQLITE3
SQLITE3 can just be installed from the Ubuntu sources:

$ sudo apt-get install sqlite3 libsqlite3-dev

That's it - done.


#### Installing BEAR
BEAR can just be installed from the Ubuntu sources:

$ sudo apt-get install bear

Done!


#### Installing PYTHON3
Python3 can be installed using the Ubuntu sources as well. Just use
the command:

$ sudo apt-get install python3

and you are done.


#### Installing DOXYGEN and GRAPHVIZ (Required for generating the documentation)
If you want to generate the documentation running 'make doc' you have to install 
[Doxygen](www.doxygen.org) and [Graphviz](www.graphviz.org).
To install them just use the command:
  
  $ sudo apt-get install doxygen graphviz

Done!


#### Installing BOOST
First you have to download the BOOST source files. This can be achieved by:

$ wget https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.gz

Next the archive file must be extracted by using:

$ tar xvf boost_1_64_0.tar.gz

Jump into the directory!

$ cd boost_1_64_0/

The next command which prepares the compilation process assumes that you have
write permission in your system's /usr/local/ directory.

$ ./boostrap.sh

If no errors occur boost can now be installed using admin permission:

$ sudo ./b2 install

You should be able to find the boost installation on your system now.

$ ls /usr/local/lib

Should output a lot of library files prefixed with 'libboost_'.
The result of the command

$ ls /usr/local/include

should contain one directory which is called 'boost'. Congratulations, now you 
have installed boost. The hardest part is yet to come.

#### Installing LLVM
When installing LLVM your best bet is probably to install it by using the apt packages.
First add the llvm-3.9 repository using the following command which is specific for Ubuntu 16.04
if you have a different version of Ubuntu, please change the following command to your needs!
You can find the details for that on the llvm webpage [http://apt.llvm.org/](http://apt.llvm.org/):

$ sudo add-apt-repository 'deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-3.9 main'

$ sudo apt-get update

Next you need to get the archives signature using:

$ wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -

(Fingerprint: 6084 F3CF 814B 57C1 CF12 EFD5 15CF 4D18 AF4F 7421)

Almost done, now you should be able to install all llvm related packages by using the following
command:

$ apt-get install clang-3.9 clang-3.9-doc libclang-common-3.9-dev libclang-3.9-dev libclang1-3.9 libclang1-3.9-dbg libllvm-3.9-ocaml-dev libllvm3.9 libllvm3.9-dbg lldb-3.9 llvm-3.9 llvm-3.9-dev llvm-3.9-doc llvm-3.9-examples llvm-3.9-runtime clang-format-3.9 python-clang-3.9 libfuzzer-3.9-dev

(If your system complains about missing dependencies for the above packages, install them as suggested by your system. For instance, we know that on some systems libz must be installed, if not installed already.)

It is important that the corresponding include/ and lib/ directories are added 
to the search path, such that ourframework is able to find them (but that is automatically the case if you perform the installation using apt packages). Important: The llvm / clang tools must be added to the search path using the following command:

+ export PATH="/usr/lib/llvm-3.9/bin:$PATH"

This line should be added to the end of your .bashrc as well. You can use this command:

$ echo -e "export PATH="/usr/lib/llvm-3.9/bin:$PATH" >> .bashrc

Almost done! After having everything set-up correctly, you can now continue the
installation by compiling ourframework. For the sake of compilation we provide 
two mechanisms:

#### Makefile
Just type 'make' and ourframework will be compiled. As usual 'make clean' will 
delete all compiled and auto-generated files. Using 'make doc' will generate the
doxygen code documentation. The compiled binary file can be found in the bin/ 
directory.

#### CMake
If you are a fan of cmake you probably would like to go this route. 
The following commands will do the job:

$ mkdir build

$ cd build/

$ cmake ..

$ make -j $(nproc)

After compilation using cmake the binary can be found right in the build 
directory.

Use the command:

$ bin/main --help

in order to display the manual and help message.

Please be careful and check if errors occure during the compilation of our framework.

After having compiled ourframework running small test example seems adequate.
If errors occur when running the test example your compiler might be 
misconfigured or worse (please report if that happens).

#### A remark on compile time
C++'s long compile times are always a pain. As shown in the above, when using cmake 
the compilation can run in parallel, resulting in shorter compilation times. 
We plan to adjust our handwritten Makefile as well, so that it can be build using 
makes parallelization capabilities (currently it cannot run in parallel).

#### Creating the configuration files
Before running ourframework you have to create some configuration files. Do not worry, that can be done automatically. To do that please run the following commands:

$ cd misc/

$ ./make_config.sh

Done!

##### Testing single modules
To test if everything works as expected please run the following commands:

$ bin/main --module test_examples/installation_tests/module.cpp --analysis ifds_uninit --wpa 1

This is to check if the internal compile mechanism is working.

$ bin/main --module test_examples/installation_tests/module.ll --analysis ifds_uninit --wpa 1

Here we check if pre-compiled modules work as expected.

##### Testing whole projects
C and C++ are notoriously hard to analyze. Because of the weak module system it is hard 
to tell which files belong to a specific project even! Usually every project
comes with a build system. Often cmake or make is used to tell the compile how to compile
the project. From these mechanisms a database containing the compile commands can be 
generated. When you compile your project using cmake just add the additional switch 
-DCMAKE_EXPORT_COMPILE_COMMANDS=1 in order to generate a file named 'compile_commands.json'.
If your project uses the Makefile mechanism you can use the 'bear' tool with which you prefix your
make command:

$ bear make

Once the 'compile_commands.json' file has been created continue with the next test.
In order to check if the digest of whole projects works as well, please try:

$ cd test_examples/installtion_test/project

$ bear make

$ cd -

$ bin/main --project test_examples/installation_tests/project/ --analysis ifds_uninit --wpa 1

The above commands run small test examples. If any errors occur, the program terminates abnormal or a segmentation fault is displayed please report detailed error messages to the developers.


Getting started
---------------
In the following we will describe how ourframework can be used to perform data-flow analyses.

### Using an existing analysis
The analysis that build into ourframework can be selected using the -a or 
--analysis command-line option. Note: more than one analysis can be selected to be 
executed on the code under analsis. Example:

$ bin/main -a ifds_uninit ...

$ bin/main -a ifds_uninit ifds_taint ...

If no analysis is selected, all available analyses will be executed on your code.
This is usually not what you would like to do.

Currently the following analyses are available in ourframework:

#### Uninitialized variables
TODO: describe what it does!

#### Taint analysis
TODO: describe what it does!

#### Taint analysis tracking paths
TODO: describe what it does!

#### Type analysis
TODO: describe what it does!

#### Immutability / const-ness analysis
TODO: describe what it does!


### Writing a static analysis
ourframework provides several sophisticated mechanisms that allow you the ease
of writing your own data-flow analysis.

#### Choosing an inter-procedural control-flow graph
The IFDS/ IDE framework solves a concrete analysis based on an inter-procedural control-flow
graph describing the structure of the code under analysis. Depending on the analysis you
would like to perform you can use a forward-, backward-, or bi-directional control-flow graph.
However, most of the time the 'LLVMBasedICFG' should work just fine.

If necessary it is also possible to provide you own implementation of an ICFG. In that case just provide another class for which you provide a reasonable name and place it in src/analysis/ifds_ide/icfg/. You implementation must at least implement the interface that is defined in ICFG.hh.

#### Important template parameters

The code is written in a very generic way. For that reason we use a lot of template parameters. Here we describe the most important template parameters:

* D
    - The type of the data-flow facts of your data-flow domain D. Very often you probably would like to use llvm::Value*.
* N
    - The type of nodes in you inter-procedural control-flow graph (or statements/ instructions). When using analysis on llvm IR it will always be llvm::Instruction*.
* M
    - The type of functions/ methods used by the framework. When using llvm it will be llvm::Function*.
* I
    - The type of the inter-procedural control-flow graph to be used. Usually it will be some reference to a type implementing the ICFG.hh interface. For example: LLVMBasedICFG&.
* V
    - The is the type for the second value domain of IDE problem. What this should be really depends of your concrete analysis. When using IFDS you do not have to worry about this type, since it is automatically chosen for you as:
```C++
            enum class BinaryDomain { 
                                      BOTTOM = 0,
                                      TOP = 1
            };
```
* L
    - Same as V, but only used internally in some specific classes.

#### Writing an IFDS analaysis
When you would like to write your own data-flow analysis using IFDS you basically just have
to implement a single class. Is is a good idea to create a new directory for your new analysis
that lives below 'ifds_ide_problems' and is name after the naming conventions that you will find 
there and contains the name of the analysis in one form or another.

To make your class an analysis problem our solver is able to solve, you let your class inherit from
'DefaultIFDSTabulationProblem'. The concrete analysis is formulated by overwriting all 
abstract functions of the 'DefaultIFDSTabulationProblem'. The member functions you have to override are:

* getNormalFlowFunction()
    - Here you formulate you flow function(s) that is (are) applied to every instruction within
      a functions body.

* getCallFlowFunction()
    - Here you express what the solver should do when it hits a call-site. In this flow function 
      the actual parameters are usually mapped to the formal parameters of the called function.

* getRetFlowFunction()
    - Here you usually propagate the information at the end of the callee back to the caller.

* getCallToRetFlowFunction()
    - Every information that is not touched by the function called is handled here. Usually
      you just want to pass every information as identity.

* Optional: getSummaryFlowFunction()
    - TODO add description

* initialSeeds()
    - The initial seeds are the starting points of your analysis. An analysis can start at one 
      or more points in your program. The functions must return start points as well as a set 
      of data-flow facts that hold at these program points. Unless your analysis requires otherwise
      you would just give the first instruction of the main function as a starting point and 
      use the special zero fact that holds at the analysis start.

* createZeroValue()
    - This function must define what value your analysis should use as the special zero value.

* Constructor
    - The constructor of you analysis receives the ICFG that shall be used for this analysis
      as a parameter. Furthermore, the constructor of the DefaultIFDSTabulationProblem that you
      inherit from must be called AND the special zeroValue must be initialized in a suitable way.
      Here is an example of how your constructor usually looks like:

```C++
            MyAnalysis::MyAnalysis(LLVMBasedICFG &icfg, llvm::LLVMContext &c) :
                                           DefaultIFDSTabulationProblem(icfg), context(c) {
                DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
            }
```

#### Writing an IDE analysis
When writing an IDE analysis you only have to implement a single class as well.
The general concept is very similar to writing an IFDS analysis. But this time your analysis
class has to inherit from 'DefaultIDETabulationProblem'.

The member functions you have to provide some implementations for are:

* getNormalFlowFunction()
    - See Writing an IFDS analysis

* getCallFlowFuntion()
    - See writing an IFDS analysis

* getRetFlowFunction()
    - See writing an IFDS analysis

* getCallToRetFlowFunction()
    - See writing an IFDS analysis

* Optional: getSummaryFlowFunction()
    - see writing an IFDS analysis

* initialSeeds()
    - See writing an IFDS analysis

* topElement()
    - A function that returns the top element of the lattice the analysis is using.

* bottomElement()
    - A function that returns the bottom element of the lattice the analysis is using.

* join()
    - A function that defines how information is joined (the merge operator of the lattice),
      which gets you higher up in the lattice. 

* allTopFunction()
    - Function that returns the special allTop edge function.

* getNormalEdgeFunction()
    - Here you formulate your edge function(s) that is (are) applied to every instruction within
	  a functions body.

* getCallEdgeFunction() 
    - Here you express what the solver should do when it hits a call-site. In this edge function 
      the actual parameters are usually mapped to the formal parameters of the called function.

* getReturnEdgeFunction() 
    - Express the edge function that is applied to map data-flow facts that hold at the 
      end of a callee back to the caller.

* getCallToReturnEdgeFunction()
    - Here the edge function is defined that is applied for data-flow facts that 'go around'
      the call-site.

* Constructor
    - See Constructor of Writing an IFDS analysis
