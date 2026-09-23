cmake_minimum_required(VERSION 3.14...3.28)

project(export-callsite-cg)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(phasar REQUIRED CONFIG)

add_executable(export-callsite-cg-streaming export_callsite_cg_streaming.cpp)
target_link_libraries(export-callsite-cg-streaming PRIVATE phasar::phasar)
