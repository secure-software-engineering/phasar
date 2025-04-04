cmake_minimum_required(VERSION 3.14...3.28)

project(phasar-how-tos)

file(GLOB children RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)

foreach(child ${children})
  if(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${child})
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/${child})
  endif()
endforeach()
