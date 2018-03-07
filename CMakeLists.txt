cmake_minimum_required (VERSION 3.0)

# Specify the build options
option(BUILD_UNITTESTS "Build all tests (default is OFF)" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries (default is ON)" ON)
option(BUILD_DOC "Build documentation" ON)
set(BUILD_MODE "Debug" CACHE STRING "Build mode ('Debug' or 'Release', default is 'Debug')")

# Set-up the compiler
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
# Set the default install prefix
set(CMAKE_INSTALL_PREFIX /home/philipp/Schreibtisch/tmp)

project (phasar)
set(CMAKE_PROJECT_NAME "phasar")

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(BUILD_MODE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-return-type-c-linkage -stdlib=libstdc++ -MP -fPIC -fvisibility-inlines-hidden -fstack-protector-strong -ffunction-sections -fdata-sections -pipe -g")
else()
  set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-return-type-c-linkage -stdlib=libstdc++ -MP -fPIC -fvisibility-inlines-hidden -fstack-protector-strong -ffunction-sections -fdata-sections -pipe -g -O2 -march=native -DNDEBUG")
endif()

# Enable testing
enable_testing()

# TODO: allow Phasar to be build as a llvm drop-in as well
# if (NOT DEFINED LLVM_MAIN_SRC_DIR)
#  message(FATAL_ERROR "Phasar is not a llvm drop-in, abort!")
# endif()

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
add_subdirectory(external/googletest EXCLUDE_FROM_ALL)
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

# Install Phasar executable and libraries
install(TARGETS llvm-phasar
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)
# Install Phasar include directory
install(DIRECTORY include/
  DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
# Install Phasar utils
install(DIRECTORY utils/
  DESTINATION bin
  FILES_MATCHING PATTERN "phasar-*"
)

# If the Phasar shared object libraries are not installed into a system folder
# the so libs must be added manually to the linker search path and the linker 
# config must be updated as follows:
#
#   $ export LD_LIBRARY_PATH=/lib:/usr/lib:/usr/local/lib:/home/philipp/Schreibtisch/tmp/lib
#   $ sudo ldconfig
#
# Or even better: just link statically when trying to package Phasar

# Settings for building various packages using Cpack
#   How to pack using the following settings?
#     $ mkdir build
#     $ cd build
#     $ cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ..
#     $ cpack ..
#     $ dpkg -i <the_package>.deb
set(MAJOR_VERSION 0)
set(MINOR_VERSION 8)
set(PATCH_VERSION 0)
IF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
INCLUDE(InstallRequiredSystemLibraries)
set(CPACK_SET_DESTDIR "on")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_DESCRIPTION "Phasar")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Phasar a LLVM-based static analysis framework")
set(CPACK_PACKAGE_VENDOR "Phasar Team - Philipp Schubert and others")
set(CPACK_PACKAGE_CONTACT "philipp.schubert@upb.de")
set(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "${PATCH_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${MAJOR_VERSION}.${MINOR_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}")
# package dependencies can be set-up here
# better use autogenerated dependency information
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost_program_options (>= 1.66.0),
#                                   libboost_filesystem (>= 1.66.0),
#                                   libboost_graph (>= 1.66.0),
#                                   libboost_system (>= 1.66.0),
#                                   libboost_log (>= 1.66.0),
#                                   libboost_thread (>= 1.66.0),
#                                   libsqlite3 (>= 4.5.0),
#                                   libpthread (>= 4.5.0),
#                                   libdl (>= 4.5.0),
#                                   libcurl-gnutls (>= 4.5.0),
#                                   libmysqlcppconn (>= 4.5.0),
#                                   librt (>= 4.5.0),
#                                   libtinfo (>= 4.5.0),
#                                   libz (>= 4.5.0),
#                                   libm (>= 4.5.0),
#                                   libstdc++ (>= 4.5.0),
#                                   libgcc_s (>= 4.5.0),
#                                   libc (>= 4.5.0),
#                                   ld-linux-x86-64 (>= 4.5.0)")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SECTION "kde")
set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
set(CPACK_COMPONENTS_ALL Libraries ApplicationData)
INCLUDE(CPack)
ENDIF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

# Setup the doxygen code documentation
if(BUILD_DOC)
  find_package(Doxygen)
  if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")
    add_custom_target(doc_doxygen ALL
      COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM
    )
  else(DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen need to be installed to generate the doxygen documentation.")
  endif()
endif()