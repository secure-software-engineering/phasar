cmake_minimum_required (VERSION 3.0)
project (sse_dfa_llvm)
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-std=c++14 -stdlib=libstdc++ -Wno-unknown-warning-option -O2 -fPIC -fvisibility-inlines-hidden -fstack-protector-strong -ffunction-sections -fdata-sections -O3  -pipe -g -march=native -DNDEBUG")
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

find_package(Boost COMPONENTS filesystem system program_options REQUIRED)
find_package(LLVM REQUIRED CONFIG)
find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h)
find_library(SQLITE3_LIBRARY NAMES sqlite3)
find_library(CLANG_LIBRARIES NAMES clang)

set(INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_path(CLANG_LIB_PATH clang)

add_definitions(${LLVM_DEFINITIONS})

set(STATIC_LINKING false)

set(CLANG_LIBRARIES
  clangTooling
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

set(ALL_LIBS 
  ${SQLITE3_LIBRARY}
  ${Boost_LIBRARIES}
  LLVM
  # ${llvm_libs}
  ${CLANG_LIBRARIES}
  )

include_directories(
  ${INCLUDES}
  ${SQLITE3_INCLUDE_DIR}
  ${BOOST_INCLUDE_DIR}
  ${LLVM_INCLUDE_DIRS}
  )


file(GLOB_RECURSE SRC_ANALYSIS src/analysis/*.h? src/analysis/*.cpp)
add_library( analysis STATIC ${SRC_ANALYSIS} )
target_link_libraries(analysis ${ALL_LIBS})

file(GLOB_RECURSE SRC_CLANG src/clang/*.h? src/clang/*.cpp)
add_library( clang STATIC ${SRC_CLANG} )
target_link_libraries(clang ${ALL_LIBS})

file(GLOB_RECURSE SRC_LIB src/lib/*.h? src/lib/*.cpp)
add_library( lib STATIC ${SRC_LIB} )
target_link_libraries(lib ${ALL_LIBS})

file(GLOB_RECURSE SRC_DB src/db/*.h? src/db/*.cpp)
add_library( db STATIC ${SRC_DB} )
target_link_libraries(db ${ALL_LIBS})

file(GLOB_RECURSE SRC_FLEX src/flex/*.h? src/flex/*.cpp)
add_library( flex STATIC ${SRC_FLEX} )
target_link_libraries(db ${ALL_LIBS})

file(GLOB_RECURSE SRC_UTILS src/utils/*.h? src/utils/*.cpp)
add_library( utils STATIC ${SRC_UTILS} )
target_link_libraries(utils ${ALL_LIBS})


add_executable(main  src/main.cpp)

target_link_libraries(main
  analysis clang db flex lib utils
  ${SQLITE3_LIBRARY}
  ${Boost_LIBRARIES}
  LLVM
  ${CLANG_LIBRARIES}
  )

# Test For Hexastore

# add_executable(test_hexastore src/db/Test.cpp src/db/Hexastore.cpp src/db/Hexastore.hh)

# target_link_libraries(test_hexastore
#   ${SQLITE3_LIBRARY}
#   )
