# LLVM Hello World

The "Hello, World!" program can be compiled using:

```bash
$ make
```

However, we recommend using cmake:

```bash
$ mkdir -p build && cd build
$ cmake ..
$ cmake --build .
```

"Hello, World!" reads a LLVM IR file (.ll or .bc) specified by the first
command-line argument. It then looks for the main function, iterates all of its
instructions and prints them to the command-line using an LLVM output stream.
Have a look at the comments within the source code in main.cpp.

Example use:

```bash
# Invoked from the llvm-hello-world root folder if compiled with make:
./main ./target/simple.ll

# Invoked from the llvm-hello-world/build folder if compiled with cmake:
./main ./target/simple_cpp_dbg.ll
```
