cmake_minimum_required (VERSION 3.0)

# Specify the build options
option(BUILD_UNITTESTS "Build all tests (default is OFF)" OFF)
set(BUILD_MODE "Debug" CACHE STRING "Build mode ('Debug' or 'Release', default is 'Debug')")

# Set-up the compiler
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(BUILD_MODE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Wno-unused-variable -Wno-unused-parameter -Wno-return-type-c-linkage -stdlib=libstdc++ -MP -fPIC -fvisibility-inlines-hidden -fstack-protector-strong -ffunction-sections -fdata-sections -pipe -g")
else()
  set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Wno-unused-variable -Wno-unused-parameter -Wno-return-type-c-linkage -stdlib=libstdc++ -MP -fPIC -fvisibility-inlines-hidden -fstack-protector-strong -ffunction-sections -fdata-sections -pipe -g -O2 -march=native -DNDEBUG")
endif()

project (phasar)
set(CMAKE_PROJECT_NAME "phasar")

# Enable testing
enable_testing()

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

# Add Phasar unittests
if (BUILD_UNITTESTS)
  message("Phasar unittests")
  add_subdirectory(unittests)
endif()

# Settings for building various packages using Cpack
#   How to pack using the following settings?
#     $ mkdir build
#     $ cd build
#     $ cmake -DCMAKE_INSTALL_PREFIX=/usr ..
#     $ cpack ..
#     $ dpkg -i <the_package>.deb
set(MAJOR_VERSION 0)
set(MINOR_VERSION 8)
set(PATCH_VERSION 0)
IF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
INCLUDE(InstallRequiredSystemLibraries)
set(CPACK_SET_DESTDIR "on")
set(CPACK_PACKAGING_INSTALL_PREFIX "/tmp")
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_DESCRIPTION "Phasar")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Phasar a LLVM-based static analysis framework")
set(CPACK_PACKAGE_VENDOR "Phasar - Philipp Schubert and others")
set(CPACK_PACKAGE_CONTACT "philipp.schubert@upb.de")
set(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "${PATCH_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}")
# package dependencies can be set-up here
# set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqtgui4 (>= 4.5.0), libstdc++6 (>= 4.2.1)")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SECTION "kde")
set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
set(CPACK_COMPONENTS_ALL Libraries ApplicationData)
INCLUDE(CPack)
ENDIF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
