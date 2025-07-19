# Use PhASAR as a Library

This small example shows how you can setup a CMake project that uses PhASAR as a library.
This guide assumes that you have installed PhASAR such that the `find_package` cmake command can find it.

You can choose the PhASAR components that you need in the `find_package` command.

Please take a look at the [CMakeLists.txt](./CMakeLists.txt) to see, how you can setup your project to use PhASAR with CMake's `find_package`.
To use phasar from a custom install location, you may specify the `phasar_ROOT` CMake variable to point to phasar's install directory.
