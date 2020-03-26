cmake_minimum_required (VERSION 3.0)

# Check if we build within the llvm source tree
if (DEFINED LLVM_MAIN_SRC_DIR)
  set(PHASAR_IN_TREE 1)
endif()

if (NOT PHASAR_IN_TREE)
  project (phasar)
  set(CMAKE_PROJECT_NAME "phasar")
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY TARGET_MESSAGES OFF)

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

file(STRINGS ${PHASAR_SRC_DIR}/include/phasar/Config/Version.h VERSION_NUMBER_FILE)
string(REPLACE " " ";" VERSION_NUMBER_FILE ${VERSION_NUMBER_FILE})
list(GET VERSION_NUMBER_FILE 2 VERSION_NUMBER_PHASAR)


include("phasar_macros")

option(PHASAR_BUILD_UNITTESTS "Build all tests (default is OFF)" OFF)

option(PHASAR_BUILD_OPENSSL_TS_UNITTESTS "Build OPENSSL typestate tests (require OpenSSL, default is OFF)" OFF)

option(PHASAR_BUILD_IR "Build IR test code (default is OFF)" OFF)

option(PHASAR_BUILD_DOC "Build documentation" OFF)

option(PHASAR_DEBUG_LIBDEPS "Debug internal library dependencies (private linkage)" OFF)

option(BUILD_SHARED_LIBS "Build shared libraries (default is ON)" ON)

option(PHASAR_ENABLE_WARNINGS "Enable warnings" ON)
if (PHASAR_ENABLE_WARNINGS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-return-type-c-linkage ")
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

# Fix boost_thread dependency for MacOS
if(APPLE)
  set(BOOST_THREAD thread-mt)
else()
  set(BOOST_THREAD thread)
endif()

# Boost
find_package(Boost 1.65.1 COMPONENTS filesystem graph system program_options log ${BOOST_THREAD} REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# JSON library
option(JSON_BuildTests OFF)
add_subdirectory(external/json EXCLUDE_FROM_ALL)
include_directories(external/json/single_include/)

# Googletest
if (NOT PHASAR_IN_TREE)
  add_subdirectory(external/googletest EXCLUDE_FROM_ALL)
  include_directories(external/googletest/googletest/include)
  include_directories(external/googletest/googlemock/include)
else()
  # Set llvm distributed includes for gtest header
  include_directories(${LLVM_MAIN_SRC_DIR}/utils/unittest/googletest/include)
  include_directories(${LLVM_MAIN_SRC_DIR}/utils/unittest/googlemock/include)
endif()

# WALi-OpenNWA
add_subdirectory(external/WALi-OpenNWA)
include_directories(external/WALi-OpenNWA/Source/wali/include)

# SQL
find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h)
find_library(SQLITE3_LIBRARY NAMES sqlite3)
include_directories(${SQLITE3_INCLUDE_DIR})

# LLVM
if (NOT PHASAR_IN_TREE)
  # Only search for LLVM if we build out of tree
  find_package(LLVM 9 REQUIRED CONFIG)
  find_library(LLVM_LIBRARY NAMES llvm REQUIRED)
  include_directories(${LLVM_INCLUDE_DIRS})
  link_directories(${LLVM_LIB_PATH} ${LLVM_LIBRARY_DIRS})
endif()

add_definitions(${LLVM_DEFINITIONS})

# Clang
if (NOT PHASAR_IN_TREE)
  # Only search for clang if we build out of tree
  find_library(CLANG_LIBRARY NAMES clang REQUIRED)
  link_directories(${CLANG_LIB_PATH})
endif()

if (PHASAR_IN_TREE)
  # Phasar needs clang headers, specificaly some that are generated by clangs table-gen
  include_directories(
    ${CLANG_INCLUDE_DIR}
    ${PHASAR_SRC_DIR}/../clang/include
    ${PROJECT_BINARY_DIR}/tools/clang/include
  )
endif()

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

set(LLVM_LINK_COMPONENTS
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

llvm_map_components_to_libnames(llvm_libs
  ${LLVM_LINK_COMPONENTS}
)

# phasar-based binaries
add_subdirectory(tools)

# Workaround: Remove Plugins for MacOS for now
if(APPLE)
  set(PHASAR_PLUGINS_LIB)
  message(WARNING "Plugins are currently not supported on MacOS platforms")
else()
  set(PHASAR_PLUGINS_LIB phasar_plugins)
endif()

# Add Phasar unittests and build all IR test code
if (PHASAR_BUILD_UNITTESTS)
  message("Phasar unittests")
  add_subdirectory(unittests)
  set(PHASAR_BUILD_IR ON)
endif()

# Build all IR test code
if (PHASAR_BUILD_IR)
  message("Building IR test code")
  add_subdirectory(test)
endif()

set(INCLUDE_INSTALL_DIR include/ CACHE PATH "Install dir of headers")

# Install targets of phasar-llvm, other executables, and libraries are to be
# found in the individual subdirectories of tools/

# Install Phasar include directory
install(DIRECTORY include/
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.def"
  PATTERN "*.h"
)

# Install the header only json container
install(DIRECTORY external/json/single_include/
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

install(EXPORT phasarTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/phasar
  NAMESPACE phasar::
  FILE phasarTargets.cmake
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  Config.cmake.in
  phasarConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/phasar
  PATH_VARS INCLUDE_INSTALL_DIR
  )

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/phasarConfigVersion.cmake
  VERSION 1.0.0
  COMPATIBILITY SameMajorVersion
  )

### Install Config and ConfigVersion files
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/phasarConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/phasarConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/phasar"
  )

# If the Phasar shared object libraries are not installed into a system folder
# the so libs must be added manually to the linker search path and the linker
# config must be updated as follows:
#
#   $ export LD_LIBRARY_PATH=/lib:/usr/lib:/usr/local/lib:/home/philipp/Schreibtisch/tmp/lib
#   $ sudo ldconfig
#
# Or even better: just link statically when trying to package Phasar <- this is no longer possible

# Settings for building various packages using Cpack
#   How to pack using the following settings?
#     $ mkdir build
#     $ cd build
#     $ cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/usr/local ..
#     $ cpack ..
#     $ dpkg -i ./<the_package>.deb or better: apt-get install ./<the_package>.deb
set(MAJOR_VERSION 1)
set(MINOR_VERSION 0)
set(PATCH_VERSION 0)
if (NOT PHASAR_IN_TREE)
IF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
INCLUDE(InstallRequiredSystemLibraries)
set(CPACK_SET_DESTDIR "on")
set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

#set(CPACK_GENERATOR "DEB")
set(CPACK_GENERATOR "RPM")
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
endif()

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
