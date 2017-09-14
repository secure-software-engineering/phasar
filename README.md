Data Flow Analysis in LLVM
==========================

Secure Software Engineering - Data Flow Analysis for C and C++
----------------------------------------------------------------

+ author: Philipp D. Schubert (philipp.schubert@upb.de)

Table of Contents
=================

* [Purpose of this tool](#purpose-of-this-tool)
* [Errors](#errors)
* [Installation](#installation)
    * [Brief example using an Ubuntu system](#brief-example-using-an-ubuntu-system)
        * [Installing SQLITE3](#installing-sqlite3)
        * [Installing BEAR](#installing-bear)
        * [Installing PYTHON3](#installing-python3)
        * [Installing BOOST](#installing-boost)
        * [Installing LLVM](#installing-llvm)
        * [Makefile](#makefile)
        * [CMake](#cmake)
    * [A remark on compile time](#a-remark-on-compile-time)
    * [Creating the configuration files](#creating-the-configuration-files)
    * [Testing single modules](#testing-single-modules)
    * [Testing whole projects](#testing-whole-projects)
* [Getting started](#getting-started)
    * [Choosing an existing analysis](#choosing-an-existing-analysis)
        * [IFDS_UninitializedVariables](#ifds_uninitializedvariables)
        * [IFDS_TaintAnalysis](#ifds_taintanalysis)
        * [IDE_TaintAnalysis](#ide_taintanalysis)
        * [IFDS_TypeAnalysis](#ifds_typeanalysis)
        * [IFDS_SolverTest](#ifds_solvertest)
        * [IDE_SolverTest](#ide_solvertest)
        * [Immutability / const-ness analysis (TODO fix name)](#immutability--const-ness-analysis-todo-fix-name)
        * [MONO_Intra_SolverTest](#mono_intra_solvertest)
        * [MONO_Inter_SolverTest](#mono_inter_solvertest)
        * [None](#none)
    * [Running an analysis](#running-an-analysis)
    * [A concrete example and how to interpret the results](#a-concrete-example-and-how-to-interpret-the-results)
    * [Writing a static analysis](#writing-a-static-analysis)
        * [Choosing a control-flow graph](#choosing-a-control-flow-graph)
        * [Useful shortcuts](#useful-shortcuts)
            * [The std::algorithm header](#the-stdalgorithm-header)
            * [The pre-defined flow_func classes](#the-pre-defined-flow_func-classes)
        * [Important template parameters](#important-template-parameters)
        * [Writing an intra-procedural monotone framework analysis](#writing-an-intra-procedural-monotone-framework-analysis)
        * [Writing an inter-procedural monotone framework analysis (using call-strings)](#writing-an-inter-procedural-monotone-framework-analysis-using-call-strings)
        * [Writing an IFDS analaysis](#writing-an-ifds-analaysis)
        * [Writing an IDE analysis](#writing-an-ide-analysis)


Purpose of this tool {#purpose-of-this-tool}
--------------------
TODO descibe what this is all about!

### Errors {#errors}
This framework is still under heavy development. Thus, it might contain errors that
are (un)known to the developers. If you find an error please send mail and report
it to the developers. The report should include at least a summary of what you were doing when you hit the error and a complete error message (if possible). We will try to fix bugs as
quickly as possible, please help us achieving this goal.

Installation {#installation}
------------
The installation of ourframework is not that trivial, since it has some library
dependencies. The libraries needed in order to be able to compile and run
ourframework successfully are the following (it is important that the exact
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

[ZLIB](https://zlib.net/) - a lossless data-compresion library

[LIBCURSES](http://www.gnu.org/software/ncurses/ncurses.html) - a terminal control library for constructing text user interfaces.

[Doxygen](www.doxygen.org) 

[Graphviz](www.graphviz.org)




### Brief example using an Ubuntu system
In the following we would like to give an complete example of how to install 
our framework using an Ubuntu (16.04) or Unix-like system.


#### Installing ZLIB
ZLIB can just be installed from the Ubuntu sources:

$ sudo apt-get install zlib1g-dev

That's it - done.


#### Installing LIBCURSES
LIBCURSES can just be installed from the Ubuntu sources:

$ sudo apt-get install libncurses5-dev

Done!


#### Installing SQLITE3 {#installing-sqlite3}
SQLITE3 can just be installed from the Ubuntu sources:

$ sudo apt-get install sqlite3 libsqlite3-dev

That's it - done.

#### Installing BEAR {#installing-bear}
BEAR can just be installed from the Ubuntu sources:

$ sudo apt-get install bear

Done!

#### Installing PYTHON3 {#installing-python3}
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

$ ./bootstrap.sh

If no errors occur boost can now be installed using admin permission:

$ sudo ./b2 install

You should be able to find the boost installation on your system now.

$ ls /usr/local/lib

Should output a lot of library files prefixed with 'libboost_'.
The result of the command

$ ls /usr/local/include

should contain one directory which is called 'boost'. Congratulations, now you
have installed boost. The hardest part is yet to come.

#### Installing LLVM {#installing-llvm}
When installing LLVM your best bet is probably to install it by using the apt packages.
First add the llvm-3.9 repository using the following command which is specific for Ubuntu 16.04
if you have a different version of Ubuntu, please change the following command to your needs!
You can find the details for that on the llvm webpage [http://apt.llvm.org/](http://apt.llvm.org/):

$ sudo add-apt-repository 'deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-3.9 main'

$ sudo apt-get update

Next you need to get the archives signature using:

$ wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -

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

#### Makefile {#makefile}
Just type 'make' and ourframework will be compiled. As usual 'make clean' will
delete all compiled and auto-generated files. Using 'make doc' will generate the
doxygen code documentation. The compiled binary file can be found in the bin/
directory. You can use the -j switch to build in parallel reducing the compile time.

$ make -j $(nproc)

#### CMake {#cmake}
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

#### A remark on compile time {#a-remark-on-compile-time}
C++'s long compile times are always a pain. As shown in the above, when using cmake the compilation can run in parallel, resulting in shorter compilation times. Our handwritten Makefile is able to run in parallel as well (as shown in the above).

#### Creating the configuration files {#creating-the-configuration-files}
Before running ourframework you have to create some configuration files. Do not worry, that can be done automatically. To do that please run the following commands:

$ cd misc/

$ ./make_config.sh

Done!

##### Testing single modules {#testing-single-modules}
To test if everything works as expected please run the following commands:

$ bin/main --module test_examples/installation_tests/module.cpp --analysis ifds_uninit --wpa 1

This is to check if the internal compile mechanism is working.

$ bin/main --module test_examples/installation_tests/module.ll --analysis ifds_uninit --wpa 1

Here we check if pre-compiled modules work as expected.

##### Testing whole projects {#testing-whole-projects}
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


Getting started {#getting-started}
---------------
In the following we will describe how ourframework can be used to perform data-flow analyses.

### Choosing an existing analysis {#choosing-an-existing-analysis}
The analysis that build into ourframework can be selected using the -a or
--analysis command-line option. Note: more than one analysis can be selected to be
executed on the code under analsis. Example:

$ bin/main -a ifds_uninit ...

$ bin/main -a ifds_uninit ifds_taint ...

If no analysis is selected, all available special test analyses will be executed on your code. These test analyses will check if their corresponding solver is working correctly. If such an analysis fails, there is definitely an error within the code or project under analysis or within the framework (which is obviously worse). In any way please report these errors.

Currently the following analyses are available in ourframework:

#### IFDS_UninitializedVariables {#ifds_uninitializedvariables}
TODO: describe what it does!

#### IFDS_TaintAnalysis {#ifds_taintanalysis}
TODO: describe what it does!

#### IDE_TaintAnalysis {#ide_taintanalysis}
TODO: describe what it does!

#### IFDS_TypeAnalysis {#ifds_typeanalysis}
TODO: describe what it does!

#### IFDS_SolverTest {#ifds_solvertest}
TODO: describe what it does!

#### IDE_SolverTest {#ide_solvertest}
TODO: describe what it does!

#### Immutability / const-ness analysis (TODO fix name) {#immutability--const-ness-analysis-todo-fix-name}
TODO: describe what it does!

#### MONO_Intra_SolverTest {#mono_intra_solvertest}
TODO: describe what it does!

#### MONO_Inter_SolverTest {#mono_inter_solvertest}
TODO: describe what it does!

#### None {#none}
TODO: describe what it does!

### Running an analysis {#running-an-analysis}
When you have chosen an analysis, you can run it on some code. The code on which the analysis runs can be either C/C++ or LLVM IR code. The framework can even run on whole projects.


Let us start with an easy example or analyzing some code. First let us consider some C/C++ code that a user would like to analyze. Let us assume there is a file called main.cpp with the following contents:
```C++
int main() {
    int i;
    int j = 4;
    int k = 5;
    int l = i + j;
    l += k;
    return 0;
}
```
Since all analysis solvers are working on the LLVM IR we probably want to see the corresponding LLVM IR. (By the way: the LLVM Language Reference Manual can be found [here](http://llvm.org/docs/LangRef.html)). It makes much sense to make oneself familiar with the intermediate representation that all infrastructure is based on!

The above C/C++ code can be translated into the LLVM IR using programs of the LLVM compiler tool chain. In our case we call the clang compiler and ask it to emit the LLVM IR code.

$ clang++ -emit-llvm -S main.cpp

After running this command a file named main.ll can be found within the current directory. The main.ll should contain code similar to:
```C++
; ModuleID = 'main.cpp'
source_filename = "main.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: norecurse nounwind uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  store i32 4, i32* %3, align 4
  store i32 5, i32* %4, align 4
  %6 = load i32, i32* %2, align 4
  %7 = load i32, i32* %3, align 4
  %8 = add nsw i32 %6, %7
  store i32 %8, i32* %5, align 4
  %9 = load i32, i32* %4, align 4
  %10 = load i32, i32* %5, align 4
  %11 = add nsw i32 %10, %9
  store i32 %11, i32* %5, align 4
  ret i32 0
}

attributes #0 = { norecurse nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.1-4ubuntu3~16.04.1 (tags/RELEASE_391/rc2)"}
```

It is important to recognize that all of our analysis run on the IR rather than the originally source code. The file to be analyzed by our framework can be specified using the -m flag. As default behavior ourframework starts the analysis at the very first instruction of the main() function.

An example call would be:

$ main -m path/to/your/main.ll -a ifds_solvertest

to run an IFDS solver test on the IR code contained in main.ll.

The LLVM infrastructure supports many different passes that can be run on the intermediate representation in order to optimize or simplify it. "The compiler front-end typically generates very stupid code" that becomes high quality code when using different passes. One very important pass that might be used is the so called memory to register pass (mem2reg). LLVM follows a register based design (unlike stack-based Java byte code). Conceptually LLVM assumes that it has an infinite amount of registers that it can use. (It is up to the code generator, to produce code that uses the amount of registers available for the target platform.) As you can see in our code example above we use some stack variables i, j, ... and so on. They translate into the LLVM IR as the result of alloca instructions which allocate the desired amount of stack memory. The mem2reg pass now tries to make use of the infinite amount of register in a way that it tries to eliminate as much 'memory cells' as possible and places values into registers instead. Due to complex pointer arithmetic LLVM is most of the time not able to eliminate all alloca instructions. Reducing the amount of memory cells makes analysis-writing much more easy. But it is important that one understand the conceptual step that mem2reg performs. Ourframework runs the mem2reg pass automatically as default behavior on the code under analysis. For beginners and debug reasons this behavior of ourframework can be changed to not running mem2reg by using the --mem2reg option.

Let us have a look what the mem2reg pass does to our IR code from above. We run the pass by using the opt tool provided by the compiler tool chain.

$ opt -mem2reg -S main.ll

The output of that command should look similar to

```C++
; ModuleID = 'simple.ll'
source_filename = "simple.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: norecurse nounwind uwtable
define i32 @main() #0 {
  %1 = add nsw i32 undef, 4
  %2 = add nsw i32 %1, 5
  ret i32 0
}

attributes #0 = { norecurse nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.1-4ubuntu3~16.04.1 (tags/RELEASE_391/rc2)"}
```

As expected LLVM is able to place all values into registers and can completely replace all of the alloca instructions. LLVM is also able to detect that the first addition in the unoptimized IR code uses an uninitialized variable as one of its operands, replacing it using a special undef value. Ourframework does not care if you provide C/C++/LLVM IR or even optimized IR code when using it with the -m command-line option. If the file your provide contains C or C++ code it will compile this code automatically into IR for you and runs additional required passes on it.

Ourframework is also able to analyse whole project in a whole program analysis or in a module-wise style. For simplicity we just consider the whole program analysis (WPA) mode. Imagine your project is a Makefile project which has a structure as shown in the following:

```C++
project/
    src/
    doc/
    ...
    Makefile
    ...
```
Ourframework needs to understand your project (which files belong to this project, what is compiled, what are macro definitions and so on). For that reason it needs a so called compiler command database which is usually named compile_commands.json. When using a Makefile for a project this database can be generated using the bear tool. You just prefix your call to make with bear like the following:

$ bear make

A compile_commands.json file should now show up within your project directory. Instead of calling ourframework with the -m command-line option you now have to use -p (p for project, of course) and specify the absolute path to your project directory which contains the compile_commands.json file. Ourframework will now internally compile each C or C++ file that belongs to your project into LLVM IR code that it stores in-memory. After having produced all of the IR code it next links all code together leading to one large IR module that makes up your entire program.

An example call is as simple as:

$ main -p /the/path/to/your/project/ -a ifds_solvertest

to run a simple IFDS solver test.

When you would like to analyze a project that uses a CMakeLists.txt you do not even need the bear tool. The cmake command allows specifying the flag -DCMAKE_EXPORT_COMPILE_COMMANDS=1 which will create the compile_commands.json for you. Suppose you have the following project structure:

```C++
project/
    src/
    doc/
    ...
    CMakeLists.txt
    ...
```
You can run

$ mkdir build
$ cd build
$ cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..

The generated compiler_commands.json will be placed in the current (build/) directory.

#### A concrete example and how to interpret the results {#a-concrete-example-and-how-to-interpret-the-results}
Let us consider this slightly more complex C++ program:
```C++
int function(int x, int y) {
	int i;
	int j = x;
	int k = y;
	return i+k;
}

int main(int argc, char** argv) {
	int i;
	int j;
	int k;
	k = function(j, 12);
	return 0;
}
```

The above program translates into the following IR code:

```C++
; ModuleID = 'main.cpp'
source_filename = "main.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nounwind uwtable
define i32 @_Z8functionii(i32, i32) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  store i32 %0, i32* %3, align 4
  store i32 %1, i32* %4, align 4
  %8 = load i32, i32* %3, align 4
  store i32 %8, i32* %6, align 4
  %9 = load i32, i32* %4, align 4
  store i32 %9, i32* %7, align 4
  %10 = load i32, i32* %5, align 4
  %11 = load i32, i32* %7, align 4
  %12 = add nsw i32 %10, %11
  ret i32 %12
}

; Function Attrs: norecurse nounwind uwtable
define i32 @main(i32, i8**) #1 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i8**, align 8
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  store i32 0, i32* %3, align 4
  store i32 %0, i32* %4, align 4
  store i8** %1, i8*** %5, align 8
  %9 = load i32, i32* %7, align 4
  %10 = call i32 @_Z8functionii(i32 %9, i32 12)
  store i32 %10, i32* %8, align 4
  ret i32 0
}

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { norecurse nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.1-4ubuntu3~16.04.1 (tags/RELEASE_391/rc2)"}
```

Running the [IFDS_UninitializedVariables](#ifds_uninitializedvariables) analysis on the non-mem2reg transformed code produces the following IFDS/IDE results (which are quite different from the intra/inter monotone framework results that are completely self-explaining. For that reason, we omit their explanation here.):

```C++
--- IFDS START RESULT RECORD ---
N
  %3 = alloca i32, align 4, !ourframework.id !1
of function: _Z8functionii
D
  %4 = alloca i32, align 4, !ourframework.id !2

V
  BinaryDomain::BOTTOM
D
  %3 = alloca i32, align 4, !ourframework.id !1

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %5 = alloca i32, align 4, !ourframework.id !3

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %4 = alloca i32, align 4, !ourframework.id !2
of function: _Z8functionii
D
  %3 = alloca i32, align 4, !ourframework.id !1

V
  BinaryDomain::BOTTOM
D
  %4 = alloca i32, align 4, !ourframework.id !2

V
  BinaryDomain::BOTTOM
D
  %5 = alloca i32, align 4, !ourframework.id !3

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %5 = alloca i32, align 4, !ourframework.id !3
of function: _Z8functionii
D
  %3 = alloca i32, align 4, !ourframework.id !1

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %5 = alloca i32, align 4, !ourframework.id !3

V
  BinaryDomain::BOTTOM
D
  %4 = alloca i32, align 4, !ourframework.id !2

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %6 = alloca i32, align 4, !ourframework.id !4
of function: _Z8functionii
D
  %3 = alloca i32, align 4, !ourframework.id !1

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %5 = alloca i32, align 4, !ourframework.id !3

V
  BinaryDomain::BOTTOM
D
  %4 = alloca i32, align 4, !ourframework.id !2

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %7 = alloca i32, align 4, !ourframework.id !5
of function: _Z8functionii
D
  %3 = alloca i32, align 4, !ourframework.id !1

V
  BinaryDomain::BOTTOM
D
  %4 = alloca i32, align 4, !ourframework.id !2

V
  BinaryDomain::BOTTOM
D
  %5 = alloca i32, align 4, !ourframework.id !3

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  store i32 %0, i32* %3, align 4, !ourframework.id !6
of function: _Z8functionii
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
  %5 = alloca i32, align 4, !ourframework.id !3

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %4 = alloca i32, align 4, !ourframework.id !2

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %3 = alloca i32, align 4, !ourframework.id !1

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  store i32 %1, i32* %4, align 4, !ourframework.id !7
of function: _Z8functionii
D
  %4 = alloca i32, align 4, !ourframework.id !2

V
  BinaryDomain::BOTTOM
D
  %5 = alloca i32, align 4, !ourframework.id !3

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %8 = load i32, i32* %3, align 4, !ourframework.id !8
of function: _Z8functionii
D
  %5 = alloca i32, align 4, !ourframework.id !3

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  store i32 %8, i32* %6, align 4, !ourframework.id !9
of function: _Z8functionii
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %5 = alloca i32, align 4, !ourframework.id !3

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %9 = load i32, i32* %4, align 4, !ourframework.id !10
of function: _Z8functionii
D
  %5 = alloca i32, align 4, !ourframework.id !3

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  store i32 %9, i32* %7, align 4, !ourframework.id !11
of function: _Z8functionii
D
  %5 = alloca i32, align 4, !ourframework.id !3

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %10 = load i32, i32* %5, align 4, !ourframework.id !12
of function: _Z8functionii
D
  %5 = alloca i32, align 4, !ourframework.id !3

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %11 = load i32, i32* %7, align 4, !ourframework.id !13
of function: _Z8functionii
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
D
  %10 = load i32, i32* %5, align 4, !ourframework.id !12

V
  BinaryDomain::BOTTOM
D
  %5 = alloca i32, align 4, !ourframework.id !3

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %12 = add nsw i32 %10, %11, !ourframework.id !14
of function: _Z8functionii
D
  %10 = load i32, i32* %5, align 4, !ourframework.id !12

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
D
  %5 = alloca i32, align 4, !ourframework.id !3

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  ret i32 %12, !ourframework.id !15
of function: _Z8functionii
D
  %5 = alloca i32, align 4, !ourframework.id !3

V
  BinaryDomain::BOTTOM
D
  %10 = load i32, i32* %5, align 4, !ourframework.id !12

V
  BinaryDomain::BOTTOM
D
  %12 = add nsw i32 %10, %11, !ourframework.id !14

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %3 = alloca i32, align 4, !ourframework.id !1
of function: main
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %4 = alloca i32, align 4, !ourframework.id !2
of function: main
D
  %3 = alloca i32, align 4, !ourframework.id !1

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %8 = alloca i32, align 4, !ourframework.id !6

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %5 = alloca i8**, align 8, !ourframework.id !3
of function: main
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %8 = alloca i32, align 4, !ourframework.id !6

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %3 = alloca i32, align 4, !ourframework.id !1

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %6 = alloca i32, align 4, !ourframework.id !4
of function: main
D
  %3 = alloca i32, align 4, !ourframework.id !1

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %8 = alloca i32, align 4, !ourframework.id !6

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %7 = alloca i32, align 4, !ourframework.id !5
of function: main
D
  %3 = alloca i32, align 4, !ourframework.id !1

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %8 = alloca i32, align 4, !ourframework.id !6

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %8 = alloca i32, align 4, !ourframework.id !6
of function: main
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
D
  %8 = alloca i32, align 4, !ourframework.id !6

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %3 = alloca i32, align 4, !ourframework.id !1

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  store i32 0, i32* %3, align 4, !ourframework.id !7
of function: main
D
  %3 = alloca i32, align 4, !ourframework.id !1

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %8 = alloca i32, align 4, !ourframework.id !6

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  store i32 %0, i32* %4, align 4, !ourframework.id !8
of function: main
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %8 = alloca i32, align 4, !ourframework.id !6

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  store i8** %1, i8*** %5, align 8, !ourframework.id !9
of function: main
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %8 = alloca i32, align 4, !ourframework.id !6

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
of function: main
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
D
  %8 = alloca i32, align 4, !ourframework.id !6

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  %10 = call i32 @_Z8functionii(i32 %9, i32 12), !ourframework.id !11
of function: main
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %8 = alloca i32, align 4, !ourframework.id !6

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  store i32 %10, i32* %8, align 4, !ourframework.id !12
of function: main
D
  %8 = alloca i32, align 4, !ourframework.id !6

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
D
  store i32 %10, i32* %8, align 4, !ourframework.id !12

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
--- IFDS START RESULT RECORD ---
N
  ret i32 0, !ourframework.id !13
of function: main
D
  %6 = alloca i32, align 4, !ourframework.id !4

V
  BinaryDomain::BOTTOM
D
  %7 = alloca i32, align 4, !ourframework.id !5

V
  BinaryDomain::BOTTOM
D
  %9 = load i32, i32* %7, align 4, !ourframework.id !10

V
  BinaryDomain::BOTTOM
D
  store i32 %10, i32* %8, align 4, !ourframework.id !12

V
  BinaryDomain::BOTTOM
D
@zero_value = constant i2 0, align 4

V
  BinaryDomain::BOTTOM
```

In IFDS/IDE results for each program statement N, all data-flow facts D holding at this program point are shown. Additionally the value from the value domain V is printed. Note: when running IFDS analysis, only BOTTOM is shown, since TOP is representing data-flow facts that do not hold and thus are irrelevant to the analysis user.

Additionally to the results, ourframe is able to record all edges from the exploded super-graph that the computation is based on. The edges reside in two edge recorders (for intra- and inter-procedural edges) inside the IDESolver implementation. For the above example the following exploded super-graph is produced:

```C++
COMPUTED INTRA PATH EDGES
FROM
  %3 = alloca i32, align 4, !ourframework.id !1
TO
  %4 = alloca i32, align 4, !ourframework.id !2
FACTS
fact
  %3 = alloca i32, align 4, !ourframework.id !1
produces
  %3 = alloca i32, align 4, !ourframework.id !1
fact
  %4 = alloca i32, align 4, !ourframework.id !2
produces
  %4 = alloca i32, align 4, !ourframework.id !2
fact
  %5 = alloca i32, align 4, !ourframework.id !3
produces
  %5 = alloca i32, align 4, !ourframework.id !3
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %4 = alloca i32, align 4, !ourframework.id !2
TO
  %5 = alloca i32, align 4, !ourframework.id !3
FACTS
fact
  %3 = alloca i32, align 4, !ourframework.id !1
produces
  %3 = alloca i32, align 4, !ourframework.id !1
fact
  %4 = alloca i32, align 4, !ourframework.id !2
produces
  %4 = alloca i32, align 4, !ourframework.id !2
fact
  %5 = alloca i32, align 4, !ourframework.id !3
produces
  %5 = alloca i32, align 4, !ourframework.id !3
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %5 = alloca i32, align 4, !ourframework.id !3
TO
  %6 = alloca i32, align 4, !ourframework.id !4
FACTS
fact
  %3 = alloca i32, align 4, !ourframework.id !1
produces
  %3 = alloca i32, align 4, !ourframework.id !1
fact
  %4 = alloca i32, align 4, !ourframework.id !2
produces
  %4 = alloca i32, align 4, !ourframework.id !2
fact
  %5 = alloca i32, align 4, !ourframework.id !3
produces
  %5 = alloca i32, align 4, !ourframework.id !3
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %6 = alloca i32, align 4, !ourframework.id !4
TO
  %7 = alloca i32, align 4, !ourframework.id !5
FACTS
fact
  %3 = alloca i32, align 4, !ourframework.id !1
produces
  %3 = alloca i32, align 4, !ourframework.id !1
fact
  %4 = alloca i32, align 4, !ourframework.id !2
produces
  %4 = alloca i32, align 4, !ourframework.id !2
fact
  %5 = alloca i32, align 4, !ourframework.id !3
produces
  %5 = alloca i32, align 4, !ourframework.id !3
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %7 = alloca i32, align 4, !ourframework.id !5
TO
  store i32 %0, i32* %3, align 4, !ourframework.id !6
FACTS
fact
  %3 = alloca i32, align 4, !ourframework.id !1
produces
  %3 = alloca i32, align 4, !ourframework.id !1
fact
  %4 = alloca i32, align 4, !ourframework.id !2
produces
  %4 = alloca i32, align 4, !ourframework.id !2
fact
  %5 = alloca i32, align 4, !ourframework.id !3
produces
  %5 = alloca i32, align 4, !ourframework.id !3
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  store i32 %0, i32* %3, align 4, !ourframework.id !6
TO
  store i32 %1, i32* %4, align 4, !ourframework.id !7
FACTS
fact
  %3 = alloca i32, align 4, !ourframework.id !1
produces
fact
  %4 = alloca i32, align 4, !ourframework.id !2
produces
  %4 = alloca i32, align 4, !ourframework.id !2
fact
  %5 = alloca i32, align 4, !ourframework.id !3
produces
  %5 = alloca i32, align 4, !ourframework.id !3
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  store i32 %1, i32* %4, align 4, !ourframework.id !7
TO
  %8 = load i32, i32* %3, align 4, !ourframework.id !8
FACTS
fact
  %4 = alloca i32, align 4, !ourframework.id !2
produces
fact
  %5 = alloca i32, align 4, !ourframework.id !3
produces
  %5 = alloca i32, align 4, !ourframework.id !3
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %8 = load i32, i32* %3, align 4, !ourframework.id !8
TO
  store i32 %8, i32* %6, align 4, !ourframework.id !9
FACTS
fact
  %5 = alloca i32, align 4, !ourframework.id !3
produces
  %5 = alloca i32, align 4, !ourframework.id !3
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  store i32 %8, i32* %6, align 4, !ourframework.id !9
TO
  %9 = load i32, i32* %4, align 4, !ourframework.id !10
FACTS
fact
  %5 = alloca i32, align 4, !ourframework.id !3
produces
  %5 = alloca i32, align 4, !ourframework.id !3
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %9 = load i32, i32* %4, align 4, !ourframework.id !10
TO
  store i32 %9, i32* %7, align 4, !ourframework.id !11
FACTS
fact
  %5 = alloca i32, align 4, !ourframework.id !3
produces
  %5 = alloca i32, align 4, !ourframework.id !3
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  store i32 %9, i32* %7, align 4, !ourframework.id !11
TO
  %10 = load i32, i32* %5, align 4, !ourframework.id !12
FACTS
fact
  %5 = alloca i32, align 4, !ourframework.id !3
produces
  %5 = alloca i32, align 4, !ourframework.id !3
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %10 = load i32, i32* %5, align 4, !ourframework.id !12
TO
  %11 = load i32, i32* %7, align 4, !ourframework.id !13
FACTS
fact
  %5 = alloca i32, align 4, !ourframework.id !3
produces
  %5 = alloca i32, align 4, !ourframework.id !3
  %10 = load i32, i32* %5, align 4, !ourframework.id !12
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %11 = load i32, i32* %7, align 4, !ourframework.id !13
TO
  %12 = add nsw i32 %10, %11, !ourframework.id !14
FACTS
fact
  %5 = alloca i32, align 4, !ourframework.id !3
produces
  %5 = alloca i32, align 4, !ourframework.id !3
fact
  %10 = load i32, i32* %5, align 4, !ourframework.id !12
produces
  %10 = load i32, i32* %5, align 4, !ourframework.id !12
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %12 = add nsw i32 %10, %11, !ourframework.id !14
TO
  ret i32 %12, !ourframework.id !15
FACTS
fact
  %5 = alloca i32, align 4, !ourframework.id !3
produces
  %5 = alloca i32, align 4, !ourframework.id !3
fact
  %10 = load i32, i32* %5, align 4, !ourframework.id !12
produces
  %10 = load i32, i32* %5, align 4, !ourframework.id !12
  %12 = add nsw i32 %10, %11, !ourframework.id !14
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %3 = alloca i32, align 4, !ourframework.id !1
TO
  %4 = alloca i32, align 4, !ourframework.id !2
FACTS
fact
@zero_value = constant i2 0, align 4
produces
  %3 = alloca i32, align 4, !ourframework.id !1
  %6 = alloca i32, align 4, !ourframework.id !4
  %7 = alloca i32, align 4, !ourframework.id !5
  %8 = alloca i32, align 4, !ourframework.id !6
@zero_value = constant i2 0, align 4
FROM
  %4 = alloca i32, align 4, !ourframework.id !2
TO
  %5 = alloca i8**, align 8, !ourframework.id !3
FACTS
fact
  %3 = alloca i32, align 4, !ourframework.id !1
produces
  %3 = alloca i32, align 4, !ourframework.id !1
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %8 = alloca i32, align 4, !ourframework.id !6
produces
  %8 = alloca i32, align 4, !ourframework.id !6
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %5 = alloca i8**, align 8, !ourframework.id !3
TO
  %6 = alloca i32, align 4, !ourframework.id !4
FACTS
fact
  %3 = alloca i32, align 4, !ourframework.id !1
produces
  %3 = alloca i32, align 4, !ourframework.id !1
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %8 = alloca i32, align 4, !ourframework.id !6
produces
  %8 = alloca i32, align 4, !ourframework.id !6
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %6 = alloca i32, align 4, !ourframework.id !4
TO
  %7 = alloca i32, align 4, !ourframework.id !5
FACTS
fact
  %3 = alloca i32, align 4, !ourframework.id !1
produces
  %3 = alloca i32, align 4, !ourframework.id !1
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %8 = alloca i32, align 4, !ourframework.id !6
produces
  %8 = alloca i32, align 4, !ourframework.id !6
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %7 = alloca i32, align 4, !ourframework.id !5
TO
  %8 = alloca i32, align 4, !ourframework.id !6
FACTS
fact
  %3 = alloca i32, align 4, !ourframework.id !1
produces
  %3 = alloca i32, align 4, !ourframework.id !1
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %8 = alloca i32, align 4, !ourframework.id !6
produces
  %8 = alloca i32, align 4, !ourframework.id !6
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %8 = alloca i32, align 4, !ourframework.id !6
TO
  store i32 0, i32* %3, align 4, !ourframework.id !7
FACTS
fact
  %3 = alloca i32, align 4, !ourframework.id !1
produces
  %3 = alloca i32, align 4, !ourframework.id !1
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %8 = alloca i32, align 4, !ourframework.id !6
produces
  %8 = alloca i32, align 4, !ourframework.id !6
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  store i32 0, i32* %3, align 4, !ourframework.id !7
TO
  store i32 %0, i32* %4, align 4, !ourframework.id !8
FACTS
fact
  %3 = alloca i32, align 4, !ourframework.id !1
produces
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %8 = alloca i32, align 4, !ourframework.id !6
produces
  %8 = alloca i32, align 4, !ourframework.id !6
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  store i32 %0, i32* %4, align 4, !ourframework.id !8
TO
  store i8** %1, i8*** %5, align 8, !ourframework.id !9
FACTS
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %8 = alloca i32, align 4, !ourframework.id !6
produces
  %8 = alloca i32, align 4, !ourframework.id !6
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  store i8** %1, i8*** %5, align 8, !ourframework.id !9
TO
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
FACTS
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %8 = alloca i32, align 4, !ourframework.id !6
produces
  %8 = alloca i32, align 4, !ourframework.id !6
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
TO
  %10 = call i32 @_Z8functionii(i32 %9, i32 12), !ourframework.id !11
FACTS
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
  %8 = alloca i32, align 4, !ourframework.id !6
produces
  %8 = alloca i32, align 4, !ourframework.id !6
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %10 = call i32 @_Z8functionii(i32 %9, i32 12), !ourframework.id !11
TO
  store i32 %10, i32* %8, align 4, !ourframework.id !12
FACTS
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %8 = alloca i32, align 4, !ourframework.id !6
produces
  %8 = alloca i32, align 4, !ourframework.id !6
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  store i32 %10, i32* %8, align 4, !ourframework.id !12
TO
  ret i32 0, !ourframework.id !13
FACTS
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
  %6 = alloca i32, align 4, !ourframework.id !4
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
  %7 = alloca i32, align 4, !ourframework.id !5
fact
  %8 = alloca i32, align 4, !ourframework.id !6
produces
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
  store i32 %10, i32* %8, align 4, !ourframework.id !12
produces
  store i32 %10, i32* %8, align 4, !ourframework.id !12
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
COMPUTED INTER PATH EDGES
FROM
  ret i32 %12, !ourframework.id !15
TO
  store i32 %10, i32* %8, align 4, !ourframework.id !12
FACTS
fact
  %5 = alloca i32, align 4, !ourframework.id !3
produces
fact
  %10 = load i32, i32* %5, align 4, !ourframework.id !12
produces
fact
  %12 = add nsw i32 %10, %11, !ourframework.id !14
produces
  store i32 %10, i32* %8, align 4, !ourframework.id !12
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
fact
@zero_value = constant i2 0, align 4
produces
@zero_value = constant i2 0, align 4
FROM
  %10 = call i32 @_Z8functionii(i32 %9, i32 12), !ourframework.id !11
TO
  %3 = alloca i32, align 4, !ourframework.id !1
FACTS
fact
  %6 = alloca i32, align 4, !ourframework.id !4
produces
fact
  %7 = alloca i32, align 4, !ourframework.id !5
produces
fact
  %8 = alloca i32, align 4, !ourframework.id !6
produces
fact
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
produces
  %9 = load i32, i32* %7, align 4, !ourframework.id !10
fact
@zero_value = constant i2 0, align 4
produces
  %3 = alloca i32, align 4, !ourframework.id !1
  %4 = alloca i32, align 4, !ourframework.id !2
  %5 = alloca i32, align 4, !ourframework.id !3
  %6 = alloca i32, align 4, !ourframework.id !4
  %7 = alloca i32, align 4, !ourframework.id !5
@zero_value = constant i2 0, align 4
```

We visualized the recorded edges in the following figure in order to obtain the actual exploded super-graph.

![alt text](img/ifds_uninit_exploded_supergraph/example_exploded_supergraph.png "Visualization of the recorded edges")



### Writing a static analysis {#writing-a-static-analysis}
ourframework provides several sophisticated mechanisms that allow you to
write your own data-flow analysis. In general, ourframework is designed in such a way that the analysis writer has to choose from several possible interfaces which he can use for a concrete static analysis. Depending on your analysis problem some interfaces are more suited than others. When having found the right interface for your problem, the analysis writer usually just has to provide a new class which implements the interfaces missing functionality which then in turn is the problem description for the analysis. This concrete problem description is then handed over to a corresponding solver, which solves the problem in a completely automatic fashion. In the following the possible data-flow solvers are ordered according to their power (and difficulty to provide an analysis for).

#### Choosing a control-flow graph {#choosing-a-control-flow-graph}
In general, all data-flow analysis is performed on the codes control flow graph. When writing an analysis a user has to choose a control flow graph for his analysis to work. For that reason all of our solvers work either on the CFG.hh (intra-procedural control flow graph) or on the ICFG.hh (inter-procedural control flow graph) interface.

For instance, when writing a simple intra-procedural data-flow analysis using the monotone framework the use must use one of CFG.hh's concrete implementation or provide his own implementation for this interface. Usually the pre-implemented LLVMBasedCFG.hh should do the job and can be used out-of-the-box.

The inter-procedural call-string approach and the IFDS/ IDE frameworks solve a concrete analysis based on an inter-procedural control-flow graph describing the structure of the code under analysis. Depending on the analysis needs, you can use a forward-, backward-, or bi-directional inter-procedural control-flow graph.
However, most of the time the 'LLVMBasedICFG' should work just fine.

If necessary it is also possible to provide you own implementation of an ICFG. In that case just provide another class for which you provide a reasonable name and place it in src/analysis/ifds_ide/icfg/. You implementation must at least implement the interface that is defined by the ICFG.hh interface.

#### Useful shortcuts {#useful-shortcuts}
In the following section some useful coding shortcuts are presented which may come very handy when writing a new analysis within ourframework.

##### The std::algorithm header {#the-stdalgorithm-header}
When overriding the classes for solving problems within the monotone framework, oftentimes set operations like set union, set intersect or set difference are required. Writing these functions yourself is tedious. Therefore it make much sense to use the existing set operation functions which are defined in the std::algorithm header file. Many useful functionality is provided there such as:
```C++
    ...
    std::includes /* subset */
    std::set_difference
    std::set_intersection
    std::set_union
    ...
```
The neat thing about these functions is that they are completely generic as they operating an iterators and provide several useful overloads. They work on different container types which follow the concept required by these functions.
In the following a small code example is presented:
```C++
    std::set<int> a = {1, 2, 3};
    std::set<int> b = {6, 4, 3, 2, 8, 1};
    std::set<int> c;
    std::set_union(a.begin(), a.end(),
                   b.begin(), b.end(),
                   inserter(c, c.begin()));
    bool isSubSet = std::includes(b.begin(), b.end(), a.begin(), a.end());
```

##### The pre-defined flow_func classes {#the-pre-defined-flow_func-classes}
When defining flow functions in IFDS or IDE, sometimes certain flow function type occur more than once. For instance, the Kill flow function that kills a specific data-flow fact is often needed many times. As the writer of an IFDS or IDE analysis you can find several useful pre-defined flow functions in the src/analysis/ifds_ide/flow_func/ director that can be used directly. Another very useful example is the Identity flow function. Some of these flow functions are also defined as a singleton if possible in order to keep the amount of overhead as small as possible. You can use the pre-defined flow functions inside your flow function factories using the std::make_shared().
Here is a small code example:
```C++
    // in this example let domain D be const llvm::Value*

    shared_ptr<FlowFunction<const llvm::Value*>> getNormalFlowFunction(....) {
        // check the type of the instruction
        if( ...) {
            // do some work
        } else if (...) {
            // kill some data-flow fact
            return make_shared<Kill<const llvm::Value*>>(/* some fact to be killed */);
        } else {
            // just treat everything else as Identity
            return Identity<const llvm::Value*>::v();
        }
    }
```



#### Important template parameters {#important-template-parameters}

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

#### Writing an intra-procedural monotone framework analysis {#writing-an-intra-procedural-monotone-framework-analysis}
This is probably the easiest analysis you can write and if the analysis writer is a beginner, he should definitely start at this point. Using an intra-procedural monotone framework analysis, an data-flow analysis problem can be solved within one single function (caution: function calls within the function under analysis are not followed, but the call-sites are still in the code of course). In order to formulate such an analysis the user has to implement the InraMonotoneProblem.hh interface. His implemented analysis is then handed over to the corresponding IntraMonotonSolver.hh which solves his analysis problem.


#### Writing an inter-procedural monotone framework analysis (using call-strings) {#writing-an-inter-procedural-monotone-framework-analysis-using-call-strings}
Implementation will be finished soon.

This analysis can be used when inter-procedural data-flow problems must be solved. It uses the classical monotone framework combined with the call-string approach to achieve k-context sensitivity. The k can be specified by the analysis implementor. The interface the analysis writer has to implement (InterMonotoneProblem) contains a few more functions than the IntraMonotoneProblem.hh and thus is slightly more complex. Please note that this solver has scaling problems for a large k on large programs. If the analysis writer demands a scalable analysis with infinite context sensitivity, he may would like to formulate his data-flow problem with an IFDS or IDE analysis (caution: IFDS and IDE can only be used when the flow functions used are distributive).


#### Writing an IFDS analaysis {#writing-an-ifds-analaysis}
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

#### Writing an IDE analysis {#writing-an-ide-analysis}
If you read this, you made it very far and will now explore the most complex solver we currently provide within ourframework (but do not worry, we are already planing to include another even more abstract solver).
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
