# Load LLVM IR

Shows, how you can use PhASAR to load and manage a LLVM IR module.

## Build

This example program can be built using cmake.
It assumes, that you have installed PhASAR on your system. If you did not install PhASAR to a default location, you can specify `-Dphasar_ROOT=your/path/to/phasar` when invoking `cmake`, replacing "your/path/to/phasar" by the actual path where you have installed PhASAR.

```bash
# Invoked from the 00-load-llvm-ir root folder:
$ mkdir -p build && cd build
$ cmake ..
$ cmake --build .
```

## Test

You can test the example program on the target programs from [llvm-hello-world/target](../../llvm-hello-world/target/).

```bash
# Invoked from the 00-load-llvm-ir/build folder:
./load-llvm-ir ../../../llvm-hello-world/target/simple.ll
```
