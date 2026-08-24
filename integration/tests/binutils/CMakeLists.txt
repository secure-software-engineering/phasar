cmake_minimum_required(VERSION 3.19)
project(binutils C)

# CMake derives the archiver, nm, objdump and friends from the compiler's base
# name. A wrapper named something else silently downgrades them to GNU binutils,
# which is why the compiler wrappers carry the real driver names.
# CMAKE_LINKER is deliberately absent: /usr/bin/ld is legitimate with clang.
foreach(tool CMAKE_AR CMAKE_RANLIB CMAKE_NM CMAKE_OBJDUMP)
  if(NOT "${${tool}}" MATCHES "llvm-")
    message(FATAL_ERROR "${tool} is '${${tool}}', expected an llvm- tool")
  endif()
endforeach()

add_executable(app main.c)
