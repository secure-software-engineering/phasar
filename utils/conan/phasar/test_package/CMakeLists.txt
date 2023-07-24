cmake_minimum_required(VERSION 3.1)
project(phasar-PackageTest CXX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(myphasartool myphasartool.cpp)
set_property(TARGET myphasartool PROPERTY CXX_STANDARD 17)
set_property(TARGET myphasartool PROPERTY EXPORT_COMPILE_COMMANDS YES)
set_property(TARGET myphasartool PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET myphasartool PROPERTY CXX_EXTENSIONS OFF)

target_include_directories(myphasartool PUBLIC ${CONAN_INCLUDE_DIRS})
target_link_libraries(myphasartool PUBLIC ${CONAN_LIBS})
