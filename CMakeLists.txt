cmake_minimum_required (VERSION 3.0)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -Wpedantic -Wno-unused-variable -Wno-unused-parameter -Wno-return-type-c-linkage -stdlib=libstdc++ -MP -fPIC -fvisibility-inlines-hidden -fstack-protector-strong -ffunction-sections -fdata-sections -pipe -g -march=native -DNDEBUG")

project (phasar)

# if (NOT DEFINED LLVM_MAIN_SRC_DIR)
#  message(FATAL_ERROR "Phasar is not a llvm drop-in, abort!")
# endif()

set(STATIC_LINKING false)
set(PHASAR_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PHASAR_SRC_DIR}/cmake")
include("phasar_macros")

include_directories(
  ${PHASAR_SRC_DIR}/include
)

### Adding external libraries
# Threads
find_package(Threads)

# JSON library
option(JSON_BuildTests OFF)
add_subdirectory(external/json EXCLUDE_FROM_ALL)
include_directories(external/json/src)

# Googletest
add_subdirectory(external/googletest)
include_directories(external/googletest/googletest/include)

# Boost
find_package(Boost COMPONENTS filesystem graph system program_options log thread REQUIRED)
include_directories(${BOOST_INCLUDE_DIR})
add_definitions(-DBOOST_LOG_DYN_LINK)

# Curl
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

# SQL
find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h)
find_library(SQLITE3_LIBRARY NAMES sqlite3)
include_directories(${SQLITE3_INCLUDE_DIR})

# LLVM
find_package(LLVM REQUIRED CONFIG)
find_library(LLVM_LIBRARY NAMES llvm REQUIRED)
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIB_PATH})
llvm_map_components_to_libnames(llvm_libs
  coverage
  coroutines
  libdriver
  lto
  support
  analysis
  bitwriter
  core
  ipo
  irreader
  instcombine
  instrumentation
  linker
  objcarcopts
  scalaropts
  transformutils
  codegen
  vectorize
)
add_definitions(${LLVM_DEFINITIONS})

# Clang
find_library(CLANG_LIBRARY NAMES clang REQUIRED)
link_directories(${CLANG_LIB_PATH})
set(CLANG_LIBRARIES
 clangTooling
 clangFrontendTool
 clangFrontend
 clangDriver
 clangSerialization
 clangCodeGen
 clangParse
 clangSema
 clangStaticAnalyzerFrontend
 clangStaticAnalyzerCheckers
 clangStaticAnalyzerCore
 clangAnalysis
 clangARCMigrate
 clangRewrite
 clangRewriteFrontend
 clangEdit
 clangAST
 clangASTMatchers
 clangLex
 clangBasic
 )

# Add the Phasar subdirectories
add_subdirectory(include)
add_subdirectory(lib)

# add_phasar_executable(llvm-phasar
#  tools/phasar/phasar.cpp
# )

# Build a stand-alone executable
add_executable(llvm-phasar
  tools/phasar/phasar.cpp
)
target_link_libraries(llvm-phasar
  phasar_config
  phasar_controller
  phasar_db
  phasar_experimental
  phasar_clang
  phasar_controlflow
  phasar_ifdside
  phasar_mono
  phasar_passes
  phasar_plugins
  phasar_pointer
  phasar_phasarllvm_utils
  phasar_utils
  boost_program_options
  boost_filesystem
  boost_graph
  boost_system
  boost_log
  boost_thread
  ${SQLITE3_LIBRARY}
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
  ${CLANG_LIBRARIES}  
  ${llvm_libs}
  curl
  gtest
)


#if(test)
#  file(GLOB_RECURSE test_code_files test_code/**/*.cpp)
#  set(ll_test_files "")
#  foreach(test_code_file ${test_code_files})
#    string(REGEX REPLACE ".cpp$"
#      ".ll" test_code_file_target
#      ${test_code_file})
#    add_custom_command(
#      OUTPUT ${test_code_file_target}
#      COMMAND ${CMAKE_CXX_COMPILER} -std=c++14 -S -emit-llvm ${test_code_file} -o ${test_code_file_target}
#      COMMENT "Emitting LLVM IR for ${test_code_file}."
#      VERBATIM
#      )
#    list(APPEND ll_test_files ${test_code_file_target})
#  endforeach()
#  add_custom_target(test_ll ALL
#    DEPENDS ${ll_test_files})
#  file(GLOB_RECURSE files "tests/*.cpp")
#  foreach(file ${files})
#      get_filename_component(testcase ${file} NAME_WE)
#      add_executable(${testcase} ${file})
#      set_target_properties(${testcase} PROPERTIES
#          CXX_STANDARD 14
#          CXX_STANDARD_REQUIRED ON
#      )
#
#      target_link_libraries(${testcase}
#      ${CLANG_LIBRARIES}
#      ${llvm_libs}
#      analysis phasarclang phasarlib utils config db
#      ${SQLITE3_LIBRARY}
#      ${Boost_LIBRARIES}
#      ${CMAKE_DL_LIBS}
#      ${CMAKE_THREAD_LIBS_INIT}
#      curl
#      gtest
#      )
#
#      if(NOT MSVC)
#          set_target_properties(${testcase} PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated #-Wno-float-equal -O0")
#      endif()
#
#      add_test(NAME "${testcase}"
#        COMMAND ${testcase} ${CATCH_TEST_FILTER}
#      )
#      set_tests_properties("${testcase}" PROPERTIES LABELS "all")
#  endforeach()
#  enable_testing()
#endif()
#