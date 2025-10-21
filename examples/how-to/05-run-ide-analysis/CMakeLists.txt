cmake_minimum_required(VERSION 3.14...3.28)

project(run-ide-analysis)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(phasar REQUIRED CONFIG)

add_executable(run-ide-analysis-simple simple.cpp)
target_link_libraries(run-ide-analysis-simple PRIVATE phasar::phasar)

add_executable(run-ide-analysis-helper-analyses helper-analyses.cpp)
target_link_libraries(run-ide-analysis-helper-analyses PRIVATE phasar::phasar)

add_executable(run-ide-analysis-ide-solver ide-solver.cpp)
target_link_libraries(run-ide-analysis-ide-solver PRIVATE phasar::phasar)

if (TARGET run_sample_programs)
  add_custom_target(run_run_ide_analysis
    DEPENDS run-ide-analysis-simple run-ide-analysis-helper-analyses run-ide-analysis-ide-solver
    COMMAND $<TARGET_FILE:run-ide-analysis-simple> "${CMAKE_CURRENT_LIST_DIR}/../../llvm-hello-world/target/call2.ll"
    COMMAND $<TARGET_FILE:run-ide-analysis-helper-analyses> "${CMAKE_CURRENT_LIST_DIR}/../../llvm-hello-world/target/call2.ll"
    COMMAND $<TARGET_FILE:run-ide-analysis-ide-solver> "${CMAKE_CURRENT_LIST_DIR}/../../llvm-hello-world/target/call2.ll"
  )

  add_dependencies(run_sample_programs run_run_ide_analysis)
endif()
