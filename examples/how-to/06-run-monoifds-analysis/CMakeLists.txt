cmake_minimum_required(VERSION 3.14...3.28)

project(run-monoifds-analysis)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(phasar REQUIRED CONFIG)


add_executable(run-monoifds-analysis-helper-analyses helper-analyses.cpp)
target_link_libraries(run-monoifds-analysis-helper-analyses PRIVATE phasar::phasar)

add_executable(run-monoifds-analysis-manual manual.cpp)
target_link_libraries(run-monoifds-analysis-manual PRIVATE phasar::phasar)


if (TARGET run_sample_programs)
  add_custom_target(run_run_monoifds_analysis
    DEPENDS run-monoifds-analysis-helper-analyses run-monoifds-analysis-manual LLFileGeneration
    COMMAND $<TARGET_FILE:run-monoifds-analysis-helper-analyses> "${CMAKE_CURRENT_BINARY_DIR}/../llvm-hello-world/target/taint_cpp_dbg.ll"
    COMMAND $<TARGET_FILE:run-monoifds-analysis-manual> "${CMAKE_CURRENT_BINARY_DIR}/../llvm-hello-world/target/taint_cpp_dbg.ll"
  )

  add_dependencies(run_sample_programs run_run_monoifds_analysis)
endif()
