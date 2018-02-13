cmake_minimum_required (VERSION 3.0)
project (sse_dfa_llvm)

set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wextra -Wpedantic -Wno-unused-variable -Wno-unused-parameter -Wno-return-type-c-linkage -stdlib=libstdc++ -MP -fPIC -fvisibility-inlines-hidden -fstack-protector-strong -ffunction-sections -fdata-sections -pipe -g -march=native -DNDEBUG")
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

find_package(Boost COMPONENTS filesystem graph system program_options log thread REQUIRED)
find_package(LLVM REQUIRED CONFIG)
find_package(Threads)
find_package(GTest REQUIRED)
find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h)
find_library(SQLITE3_LIBRARY NAMES sqlite3)
find_library(CLANG_LIBRARIES NAMES clang)

llvm_map_components_to_libnames(llvm_libs
  coverage libdriver lto support analysis bitwriter core ipo irreader instcombine instrumentation linker objcarcopts scalaropts transformutils codegen vectorize
)

set(INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_path(CLANG_LIB_PATH clang)

add_definitions(${LLVM_DEFINITIONS})
add_definitions(-DBOOST_LOG_DYN_LINK)

set(STATIC_LINKING false)

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

set(ALL_LIBS 
  ${SQLITE3_LIBRARY}
  ${Boost_LIBRARIES}
  # LLVM
  ${llvm_libs}
  ${CLANG_LIBRARIES}
  )

include_directories(
  ${INCLUDES}
  ${SQLITE3_INCLUDE_DIR}
  ${BOOST_INCLUDE_DIR}
  ${LLVM_INCLUDE_DIRS}
  ${GTEST_INCLUDE_DIR}
  json/src
  )


file(GLOB_RECURSE SRC_ANALYSIS src/analysis/*.h? src/analysis/*.cpp)
add_library( analysis SHARED ${SRC_ANALYSIS} )
target_link_libraries(analysis ${ALL_LIBS})

file(GLOB_RECURSE SRC_CONFIG src/config/*.h? src/config/*.cpp)
add_library( config SHARED ${SRC_CONFIG} )
target_link_libraries(config ${ALL_LIBS})

file(GLOB_RECURSE SRC_DB src/db/*.h? src/db/*.cpp)
add_library( db SHARED ${SRC_DB} )
target_link_libraries(db mysqlcppconn ${ALL_LIBS})

file(GLOB_RECURSE SRC_CLANG src/clang/*.h? src/clang/*.cpp)
add_library( clang SHARED ${SRC_CLANG} )
target_link_libraries(clang ${ALL_LIBS})

file(GLOB_RECURSE SRC_LIB src/lib/*.h? src/lib/*.cpp)
add_library( lib SHARED ${SRC_LIB} )
target_link_libraries(lib ${ALL_LIBS})

#file(GLOB_RECURSE SRC_FLEX src/flex/*.h? src/flex/*.cpp)
#add_library( flex SHARED ${SRC_FLEX} )
#target_link_libraries(db ${ALL_LIBS})

file(GLOB_RECURSE SRC_UTILS src/utils/*.h? src/utils/*.cpp)
add_library( utils SHARED ${SRC_UTILS} )
target_link_libraries(utils ${ALL_LIBS})

file(GLOB_RECURSE SRC_PLUGIN_TABTEST src/analysis/plugins/IFDSTabulationProblemTestPlugin.cxx src/analysis/plugins/IFDSTabulationProblemTestPlugin.hh)
add_library(IFDSTabulationProblemTestPlugin SHARED ${SRC_PLUGIN_TABTEST})
set_target_properties(IFDSTabulationProblemTestPlugin PROPERTIES PREFIX "")

file(GLOB_RECURSE SRC_PLUGIN_ICFGTEST src/analysis/plugins/ICFGTestPlugin.cxx src/analysis/plugins/ICFGTestPlugin.hh)
add_library(ICFGTestPlugin SHARED ${SRC_PLUGIN_TABTEST})
set_target_properties(ICFGTestPlugin PROPERTIES PREFIX "")

add_executable(main  src/main.cpp)

target_link_libraries(main
  ${CLANG_LIBRARIES}
  ${llvm_libs}
  analysis clang lib utils config db
  ${SQLITE3_LIBRARY}
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
  curl
  # LLVM
  )

# Test For Hexastore

# add_executable(test_hexastore src/db/Test.cpp src/db/Hexastore.cpp src/db/Hexastore.hh)

# target_link_libraries(test_hexastore
#   ${SQLITE3_LIBRARY}
#   )
