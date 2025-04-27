cmake_minimum_required(VERSION 3.14...3.28)

project(build-type-hierarchy)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(phasar REQUIRED CONFIG)

add_executable(build-type-hierarchy main.cpp)
target_link_libraries(build-type-hierarchy PRIVATE phasar::phasar)

if (TARGET run_sample_programs)
  add_custom_target(run_build_type_hierarchy
    DEPENDS build-type-hierarchy
    COMMAND $<TARGET_FILE:build-type-hierarchy> "${CMAKE_CURRENT_LIST_DIR}/../../llvm-hello-world/target/class_hierarchy.ll"
  )

  add_dependencies(run_sample_programs run_build_type_hierarchy)
endif()
