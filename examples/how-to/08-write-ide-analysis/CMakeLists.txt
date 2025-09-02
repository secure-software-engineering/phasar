cmake_minimum_required(VERSION 3.14...3.28)

project(write-ide-analysis)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(phasar REQUIRED CONFIG)

add_executable(write-ide-analysis-simple simple.cpp)
target_link_libraries(write-ide-analysis-simple PRIVATE phasar::phasar)


if (TARGET run_sample_programs)
  add_custom_target(run_write_ide_analysis
    DEPENDS write-ide-analysis-simple
    COMMAND $<TARGET_FILE:write-ide-analysis-simple> "${CMAKE_CURRENT_LIST_DIR}/../../llvm-hello-world/target/taint.ll"
  )

  add_dependencies(run_sample_programs run_write_ide_analysis)
endif()
