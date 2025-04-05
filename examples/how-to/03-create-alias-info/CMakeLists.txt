cmake_minimum_required(VERSION 3.14...3.28)

project(create-alias-info)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(phasar REQUIRED CONFIG)

add_executable(create-alias-info main.cpp)
target_link_libraries(create-alias-info PRIVATE phasar::phasar)
