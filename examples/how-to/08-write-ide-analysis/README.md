# Write an IDE Analysis

Shows, how you can use PhASAR to write an IDE analysis to analyze LLVM IR.
For this example, we selected the linear constant analysis as problem to implement.

The code example exactly matches the example from our Wiki: [Writing an IDE Analysis](https://github.com/secure-software-engineering/phasar/wiki/Writing-an-IDE-analysis).
So, we highly recommend taking a look there first.

## Build

This example program can be built using cmake.
It assumes, that you have installed PhASAR on your system. If you did not install PhASAR to a default location, you can specify `-Dphasar_ROOT=your/path/to/phasar` when invoking `cmake`, replacing "your/path/to/phasar" by the actual path where you have installed PhASAR.

```bash
# Invoked from the 08-write-ide-analysis root folder:
$ mkdir -p build && cd build
$ cmake ..
$ cmake --build .
```

## Test

You can test the example program on the target programs from [llvm-hello-world/target](../../llvm-hello-world/target/).

```bash
# Invoked from the 08-write-ide-analysis/build folder:
./write-ide-analysis-simple ../../../llvm-hello-world/target/call2.ll

```
