cmake_minimum_required(VERSION 3.14...3.28)

project(write-ifds-analysis)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(phasar REQUIRED CONFIG)

add_executable(write-ifds-analysis-simple simple.cpp)
target_link_libraries(write-ifds-analysis-simple PRIVATE phasar::phasar)


if (TARGET run_sample_programs)
  add_custom_target(run_write_ifds_analysis
    DEPENDS write-ifds-analysis-simple
    COMMAND $<TARGET_FILE:write-ifds-analysis-simple> "${CMAKE_CURRENT_LIST_DIR}/../../llvm-hello-world/target/taint.ll"
  )

  add_dependencies(run_sample_programs run_write_ifds_analysis)
endif()
