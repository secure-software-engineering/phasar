The "Hello, World!" program can be compiled using:

	$ make

The auto-generated files can be removed using:

	$ make clean

"Hello, World!" reads a LLVM IR file (.ll or .bc) specified by the first 
command-line argument. It then looks for the main function, iterates all of its
instructions and prints them to the command-line using an LLVM output stream.
Have a look at the comments within the source code in main.cpp.
