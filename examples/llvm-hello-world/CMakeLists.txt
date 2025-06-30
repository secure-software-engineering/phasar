cmake_minimum_required(VERSION 3.14...3.28)

project(llvm-hello-world)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM 15 REQUIRED CONFIG)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE LLVMCore LLVMIRReader)
target_include_directories(main PRIVATE ${LLVM_INCLUDE_DIRS})

add_subdirectory(target)
