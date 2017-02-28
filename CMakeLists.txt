cmake_minimum_required (VERSION 3.0)
project (sse_dfa_llvm)
set(CMAKE_CXX_STANDARD 11)

find_package(Boost REQUIRED)
find_package(LLVM REQUIRED CONFIG)
FIND_PATH(SQLITE3_INCLUDE_DIR NAMES sqlite3.h)
FIND_LIBRARY(SQLITE3_LIBRARY NAMES sqlite3)

set(INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_path(CLANG_LIB_PATH clang)

set(CLANG_LIBRARIES
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

file(GLOB_RECURSE SOURCES *.hh *.h *.cpp)

add_definitions(${LLVM_DEFINITIONS})

include_directories(
  ${INCLUDES}
  ${SQLITE3_INCLUDE_DIR}
  ${BOOST_INCLUDE_DIR}
  ${LLVM_INCLUDE_DIRS}
  )

add_executable(sse_dfa_llvm  ${SOURCES})

llvm_map_components_to_libnames(llvm_libs all)

target_link_libraries( sse_dfa_llvm
  ${SQLITE3_LIBRARY}
  ${Boost_LIBRARIES}
  ${CLANG_LIBRARIES}
  ${llvm_libs}
  )
