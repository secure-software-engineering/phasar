cmake_minimum_required(VERSION 3.14...3.28)

project(build-call-graph)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(phasar REQUIRED CONFIG)

add_executable(build-llvm-based-icfg build_llvm_based_icfg.cpp)
target_link_libraries(build-llvm-based-icfg PRIVATE phasar::phasar)

add_executable(build-llvm-based-call-graph build_llvm_based_call_graph.cpp)
target_link_libraries(build-llvm-based-call-graph PRIVATE phasar::phasar)
