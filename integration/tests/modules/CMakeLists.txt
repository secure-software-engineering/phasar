cmake_minimum_required(VERSION 3.28)
project(modules CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Module scanning needs clang-scan-deps. CMake derives its path from the compiler
# directory, where a generated wrapper has no neighbours — so the toolchain has to
# supply it. Fail loudly rather than at build time with a NOTFOUND command.
if(NOT CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS)
  message(FATAL_ERROR "CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS is not set")
endif()

add_executable(app main.cpp)
target_sources(app PRIVATE FILE_SET CXX_MODULES FILES greeting.cppm)
