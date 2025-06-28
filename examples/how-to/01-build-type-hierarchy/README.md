# Build Type Hierarchy

Shows, how you can use PhASAR to build and use a type hierarchy from a LLVM IR module.

## Build

This example program can be built using cmake.
It assumes, that you have installed PhASAR on your system. If you did not install PhASAR to a default location, you can specify `-Dphasar_ROOT=your/path/to/phasar` when invoking `cmake`, replacing "your/path/to/phasar" by the actual path where you have installed PhASAR.

```bash
# Invoked from the 01-build-type-hierarchy root folder:
$ mkdir -p build && cd build
$ cmake ..
$ cmake --build .
```

## Test

You can test the example program on the target programs from [llvm-hello-world/target](../../llvm-hello-world/target/).

```bash
# Invoked from the 01-build-type-hierarchy/build folder:
./build-type-hierarchy ../../../llvm-hello-world/target/class_hierarchy.ll
```
