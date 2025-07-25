cmake_minimum_required(VERSION 3.14...3.28)

project(create-alias-info)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(phasar REQUIRED CONFIG)

add_executable(create-alias-info main.cpp)
target_link_libraries(create-alias-info PRIVATE phasar::phasar)

if (TARGET run_sample_programs)
  add_custom_target(run_create_alias_info
    DEPENDS create-alias-info
    COMMAND $<TARGET_FILE:create-alias-info> "${CMAKE_CURRENT_LIST_DIR}/../../llvm-hello-world/target/pointers.ll"
  )

  add_dependencies(run_sample_programs run_create_alias_info)
endif()
