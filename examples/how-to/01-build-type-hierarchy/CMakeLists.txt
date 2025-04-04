cmake_minimum_required(VERSION 3.14...3.28)

project(build-type-hierarchy)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(phasar REQUIRED CONFIG)

add_executable(build-type-hierarchy main.cpp)
target_link_libraries(build-type-hierarchy PRIVATE phasar::phasar)
