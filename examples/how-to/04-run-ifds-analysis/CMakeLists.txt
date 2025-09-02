cmake_minimum_required(VERSION 3.14...3.28)

project(run-ifds-analysis)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(phasar REQUIRED CONFIG)

add_executable(run-ifds-analysis-simple simple.cpp)
target_link_libraries(run-ifds-analysis-simple PRIVATE phasar::phasar)

add_executable(run-ifds-analysis-helper-analyses helper-analyses.cpp)
target_link_libraries(run-ifds-analysis-helper-analyses PRIVATE phasar::phasar)

add_executable(run-ifds-analysis-otf-reporter otf-reporter.cpp)
target_link_libraries(run-ifds-analysis-otf-reporter PRIVATE phasar::phasar)

add_executable(run-ifds-analysis-ifds-solver ifds-solver.cpp)
target_link_libraries(run-ifds-analysis-ifds-solver PRIVATE phasar::phasar)

if (TARGET run_sample_programs)
  add_custom_target(run_run_ifds_analysis
    DEPENDS run-ifds-analysis-simple run-ifds-analysis-helper-analyses run-ifds-analysis-otf-reporter run-ifds-analysis-ifds-solver
    COMMAND $<TARGET_FILE:run-ifds-analysis-simple> "${CMAKE_CURRENT_LIST_DIR}/../../llvm-hello-world/target/taint.ll"
    COMMAND $<TARGET_FILE:run-ifds-analysis-helper-analyses> "${CMAKE_CURRENT_LIST_DIR}/../../llvm-hello-world/target/taint.ll"
    COMMAND $<TARGET_FILE:run-ifds-analysis-otf-reporter> "${CMAKE_CURRENT_LIST_DIR}/../../llvm-hello-world/target/taint.ll"
    COMMAND $<TARGET_FILE:run-ifds-analysis-ifds-solver> "${CMAKE_CURRENT_LIST_DIR}/../../llvm-hello-world/target/taint.ll"
  )

  add_dependencies(run_sample_programs run_run_ifds_analysis)
endif()
