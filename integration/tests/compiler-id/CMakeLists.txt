cmake_minimum_required(VERSION 3.19)
project(compiler_id CXX)

# Fails when CMake cannot identify the compiler — which is what happens if
# wllvm is invoked without its environment.
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL_ERROR "compiler id is '${CMAKE_CXX_COMPILER_ID}', expected Clang")
endif()

add_executable(app main.cpp)
# Needs CMAKE_CXX_COMPILE_FEATURES, so identification must have succeeded.
target_compile_features(app PRIVATE cxx_std_17)
