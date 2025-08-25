cmake_minimum_required(VERSION 3.14...3.28)

project(load-llvm-ir)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(phasar REQUIRED CONFIG)

add_executable(load-llvm-ir main.cpp)
target_link_libraries(load-llvm-ir PRIVATE phasar::phasar)

if (TARGET run_sample_programs)
  add_custom_target(run_load_llvm_ir
    DEPENDS load-llvm-ir
    COMMAND $<TARGET_FILE:load-llvm-ir> "${CMAKE_CURRENT_LIST_DIR}/../../llvm-hello-world/target/simple.ll"
  )

  add_dependencies(run_sample_programs run_load_llvm_ir)
endif()
