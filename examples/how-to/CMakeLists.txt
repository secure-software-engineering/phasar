cmake_minimum_required(VERSION 3.14...3.28)

project(phasar-how-tos)

file(GLOB children RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)

add_custom_target(run_sample_programs)

foreach(child ${children})
  if(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${child} AND NOT "${child}" STREQUAL "build")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/${child})
  endif()
endforeach()
