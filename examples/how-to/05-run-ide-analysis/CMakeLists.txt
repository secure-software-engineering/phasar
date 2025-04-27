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
