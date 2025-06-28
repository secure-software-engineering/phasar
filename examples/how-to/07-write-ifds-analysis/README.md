# Write an IFDS Analysis

Shows, how you can use PhASAR to write an IFDS analysis to analyze LLVM IR.
For this example, we selected the versatile *taint analysis* as problem to implement.

For more information, we suggest taking a look into PhASAR's Wiki: [Writing an IFDS Analysis](https://github.com/secure-software-engineering/phasar/wiki/Writing-an-IFDS-analysis).

## Build

This example program can be built using cmake.
It assumes, that you have installed PhASAR on your system. If you did not install PhASAR to a default location, you can specify `-Dphasar_ROOT=your/path/to/phasar` when invoking `cmake`, replacing "your/path/to/phasar" by the actual path where you have installed PhASAR.

```bash
# Invoked from the 07-write-ifds-analysis root folder:
$ mkdir -p build && cd build
$ cmake ..
$ cmake --build .
```

## Test

You can test the example program on the target programs from [llvm-hello-world/target](../../llvm-hello-world/target/).

```bash
# Invoked from the 07-write-ifds-analysis/build folder:
./write-ifds-analysis-simple ../../../llvm-hello-world/target/taint.ll

```
