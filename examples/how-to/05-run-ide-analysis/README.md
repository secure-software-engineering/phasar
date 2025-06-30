# Run an IDE Analysis

Shows several ways, how you can use PhASAR to run an already existing IDE analysis on a LLVM IR module.
For this example, we selected the `IDELinearConstantAnalysis`.

You may look at the different C++ source files to see, how you can run an IDE linear constant analysis using PhASAR.
We suggest to start with the simplest examples [simple.cpp](./simple.cpp) and [helper-analyses.cpp](./helper-analyses.cpp).

## Build

This example program can be built using cmake.
It assumes, that you have installed PhASAR on your system. If you did not install PhASAR to a default location, you can specify `-Dphasar_ROOT=your/path/to/phasar` when invoking `cmake`, replacing "your/path/to/phasar" by the actual path where you have installed PhASAR.

```bash
# Invoked from the 05-run-ide-analysis root folder:
$ mkdir -p build && cd build
$ cmake ..
$ cmake --build .
```

## Test

You can test the example program on the target programs from [llvm-hello-world/target](../../llvm-hello-world/target/).

```bash
# Invoked from the 05-run-ide-analysis/build folder:
./run-ide-analysis-simple ../../../llvm-hello-world/target/call2.ll

./run-ide-analysis-helper-analyses ../../../llvm-hello-world/target/call2.ll

./run-ide-analysis-ide-solver ../../../llvm-hello-world/target/call2.ll
```
