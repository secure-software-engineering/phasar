cmake_minimum_required(VERSION 3.14...3.28)

project(llvm-hello-world)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LLVM_HW_LLVM_VERSION 15)
if(PHASAR_LLVM_VERSION)
  set(LLVM_HW_LLVM_VERSION ${PHASAR_LLVM_VERSION})
endif()

message(STATUS "llvm-hello-world: Use LLVM-${LLVM_HW_LLVM_VERSION}")

find_package(LLVM ${LLVM_HW_LLVM_VERSION} REQUIRED CONFIG)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE LLVMCore LLVMIRReader)
target_include_directories(main PRIVATE ${LLVM_INCLUDE_DIRS})

add_subdirectory(target)
