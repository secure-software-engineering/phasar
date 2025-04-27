cmake_minimum_required(VERSION 3.14...3.28)

project(build-call-graph)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(phasar REQUIRED CONFIG)

add_executable(build-llvm-based-icfg build_llvm_based_icfg.cpp)
target_link_libraries(build-llvm-based-icfg PRIVATE phasar::phasar)

add_executable(build-llvm-based-call-graph build_llvm_based_call_graph.cpp)
target_link_libraries(build-llvm-based-call-graph PRIVATE phasar::phasar)

if (TARGET run_sample_programs)
  add_custom_target(run_build_call_graph
    DEPENDS build-llvm-based-icfg build-llvm-based-call-graph
    COMMAND $<TARGET_FILE:build-llvm-based-icfg> "${CMAKE_CURRENT_LIST_DIR}/../../llvm-hello-world/target/class_hierarchy.ll"
    COMMAND $<TARGET_FILE:build-llvm-based-call-graph> "${CMAKE_CURRENT_LIST_DIR}/../../llvm-hello-world/target/class_hierarchy.ll"
  )

  add_dependencies(run_sample_programs run_build_call_graph)
endif()
