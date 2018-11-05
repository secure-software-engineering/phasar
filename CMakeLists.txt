cmake_minimum_required (VERSION 3.0)

# Set-up the compiler
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project (phasar)
set(CMAKE_PROJECT_NAME "phasar")

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build mode ('Debug' or 'Release',
  default is 'Debug')" FORCE)
endif ()
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP -fvisibility-inlines-hidden -fstack-protector-strong -ffunction-sections -fdata-sections -pipe -g")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP -fvisibility-inlines-hidden -fstack-protector-strong -ffunction-sections -fdata-sections -pipe -march=native")
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

# Phasar Options
if(APPLE)
  option(PHASAR_ENABLE_LIBCXX "Use libc++ if available" ON)
else()
  option(PHASAR_ENABLE_LIBCXX "Use libc++ if available" OFF)
endif()

if(PHASAR_ENABLE_LIBCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
endif(PHASAR_ENABLE_LIBCXX)

option(PHASAR_BUILD_UNITTESTS "Build all tests (default is OFF)" OFF)

option(PHASAR_BUILD_DOC "Build documentation" OFF)

if (NOT CMAKE_BUILD_TYPE)
  option(BUILD_SHARED_LIBS "Build shared libraries (default is OFF)" OFF)
endif (NOT CMAKE_BUILD_TYPE)

option(PHASAR_ENABLE_WARNINGS "Enable warnings" ON)
if (PHASAR_ENABLE_WARNINGS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-return-type-c-linkage  ")
endif (PHASAR_ENABLE_WARNINGS)

option(PHASAR_ENABLE_PIC "Build Position-Independed Code" ON)
if (PHASAR_ENABLE_PIC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif (PHASAR_ENABLE_PIC)

if (NOT PHASAR_ENABLE_PAMM)
  set(PHASAR_ENABLE_PAMM "Off" CACHE STRING "Enable the performance measurement mechanism ('Off', 'Core' or 'Full', default is 'Off')" FORCE)
  set_property(CACHE PHASAR_ENABLE_PAMM PROPERTY STRINGS "Off" "Core" "Full")
endif()
if(PHASAR_ENABLE_PAMM STREQUAL "Core" AND NOT PHASAR_BUILD_UNITTESTS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPAMM_CORE")
  message("PAMM metric severity level: Core")
elseif(PHASAR_ENABLE_PAMM STREQUAL "Full" AND NOT PHASAR_BUILD_UNITTESTS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPAMM_FULL")
  message("PAMM metric severity level: Full")
elseif(PHASAR_BUILD_UNITTESTS)
  message("PAMM metric severity level: Off (due to unittests)")
else()
  message("PAMM metric severity level: Off")
endif()

# Workaround: Remove Plugins for MacOS for now
if(APPLE)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPHASAR_PLUGINS_ENABLED")
endif()

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
include_directories(external/googletest/googlemock/include)

# WALi-OpenNWA
add_subdirectory(external/WALi-OpenNWA)
include_directories(external/WALi-OpenNWA/Source/wali/include)

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
link_directories(${LLVM_LIB_PATH} ${LLVM_LIBRARY_DIRS})

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

# Build a stand-alone executable
add_executable(phasar
  tools/phasar/phasar.cpp
)

# Build a small test tool to show how phasar may be used
add_executable(myphasartool
  tools/phasar/myphasartool.cpp
)

# Fix boost_thread dependency for MacOS
if(APPLE)
  set(BOOST_THREAD boost_thread-mt)
else()
  set(BOOST_THREAD boost_thread)
endif()

# Workaround: Remove Plugins for MacOS for now
if(APPLE)
  set(PHASAR_PLUGINS_LIB )
  message(WARNING "Plugins are currently not supported on MacOS platforms")
else()
  set(PHASAR_PLUGINS_LIB phasar_plugins)
endif()

# Warning! There is a another listing of libraries inside cmake/phasar_macros.cmake.
# If this list is altered the other one should be altered accordingly.
target_link_libraries(phasar
  phasar_config
  phasar_controller
  phasar_db
  phasar_experimental
  phasar_clang
  phasar_controlflow
  phasar_ifdside
  phasar_mono
  phasar_passes
  ${PHASAR_PLUGINS_LIB}
  phasar_pointer
  phasar_phasarllvm_utils
  phasar_utils
  boost_program_options
  boost_filesystem
  boost_graph
  boost_system
  boost_log
  ${BOOST_THREAD}
  ${SQLITE3_LIBRARY}
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
  ${CLANG_LIBRARIES}
  ${llvm_libs}
  curl
  gtest
)

target_link_libraries(myphasartool
  phasar_config
  phasar_controller
  phasar_db
  phasar_experimental
  phasar_clang
  phasar_controlflow
  phasar_ifdside
  phasar_mono
  phasar_passes
  ${PHASAR_PLUGINS_LIB}
  phasar_pointer
  phasar_phasarllvm_utils
  phasar_utils
  boost_program_options
  boost_filesystem
  boost_graph
  boost_system
  boost_log
  ${BOOST_THREAD}
  ${SQLITE3_LIBRARY}
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
  ${CLANG_LIBRARIES}
  ${llvm_libs}
  curl
  gtest
)

# Add Phasar unittests and .ll file generation
if (PHASAR_BUILD_UNITTESTS)
  message("Phasar unittests")
  add_subdirectory(unittests)
  message("Generation of .ll files")
  add_subdirectory(test)
endif()

# Install Phasar executable and libraries
install(TARGETS phasar
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)
# Install Phasar include directory
install(DIRECTORY include/
  DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
# Install the header only json container
install(DIRECTORY external/json/src/
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp"
)
# Install the gtest header files (TODO this installation dependency should be eliminated)
install(DIRECTORY external/googletest/googletest/include/gtest/
  DESTINATION include/gtest
)
# Install Phasar utils helper scripts
install(DIRECTORY utils/
  DESTINATION bin
  FILES_MATCHING PATTERN "phasar-*"
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
              GROUP_EXECUTE GROUP_READ
              WORLD_EXECUTE WORLD_READ
)
# Install the Phasar config files into ~/.config/phasar/
install(DIRECTORY config/
  DESTINATION $ENV{HOME}/.config/phasar
  PATTERN "config/*"
  PERMISSIONS OWNER_WRITE OWNER_READ
              GROUP_WRITE GROUP_READ
              WORLD_READ
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
set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
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
if(PHASAR_BUILD_DOC)
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
