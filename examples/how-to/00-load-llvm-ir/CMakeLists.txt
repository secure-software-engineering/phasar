cmake_minimum_required(VERSION 3.14...3.28)

project(load-llvm-ir)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(phasar REQUIRED CONFIG)

add_executable(load-llvm-ir main.cpp)
target_link_libraries(load-llvm-ir PRIVATE phasar::phasar)
