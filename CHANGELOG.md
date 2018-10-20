# Improvements and API changes in Version 1118

+ Added a prototypical IDETypeState analysis that checks the correctness of the <stdio.h> usage protocol for file handles.

+ Added two new FlowFunction implementations that can be used to propagate data-flow facts on llvm::StoreInst and llvm::LoadInst automatically.

+ Set C++ standard to C++17 (Travis OSX build is disabled, until updating to a newer LLVM version as Clang's STL implementation libc++ correctly misses some features that have been removed in C++17. However, GCC's implementation libstdc++ still has them.)

+ Foundations for introducing a WPDS-based data-flow solver

+ Various bugfixes and minor improvements
