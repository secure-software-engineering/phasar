cmake_minimum_required(VERSION 3.1)
project(WALi-OpenNWA-PackageTest CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(test_package test_package.cpp)
