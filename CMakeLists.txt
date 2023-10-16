cmake_minimum_required (VERSION 3.9)

# Avoid IPO/LTO Warnings:
cmake_policy(SET CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

# Allow overwriting options of external projects from this CMakeLists file
cmake_policy(SET CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# Allow portable use of CMAKE_VISIBILITY_INLINES_HIDDEN not only for shared libraries
cmake_policy(SET CMP0063 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)

# Check if we build within the llvm source tree
if (DEFINED LLVM_MAIN_SRC_DIR)
  set(PHASAR_IN_TREE 1)
endif()

if (NOT PHASAR_IN_TREE)
  project (phasar)
  set(CMAKE_PROJECT_NAME "phasar")
endif ()

option(PHASAR_EXPERIMENTAL_CXX20 "Build phasar in C++20 mode. This is an experimental feature" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
if(PHASAR_EXPERIMENTAL_CXX20)
  message(STATUS "Selected experimental C++20 build")
  set(CMAKE_CXX_STANDARD 20)
else()
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

set_property(GLOBAL PROPERTY TARGET_MESSAGES OFF)

set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo Release CACHE STRING "Configuration types: Debug, RelWithDebInfo and Release" FORCE)

option(PHASAR_ENABLE_SANITIZERS "Build PhASAR with AddressSanitizer and UBSanitizer (default is OFF)" OFF)

set(DEBUG_CONFIGURATIONS DEBUG CACHE INTERNAL "" FORCE)
set(RELEASE_CONFIGURATIONS RELWITHDEBINFO RELEASE CACHE INTERNAL "" FORCE)

set(PHASAR_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PHASAR_SRC_DIR}/cmake")
include("phasar_macros")

if (NOT CMAKE_BUILD_TYPE AND NOT GENERATOR_IS_MULTI_CONFIG)
  message(STATUS "No CMAKE_BUILD_TYPE specified, setting it to Debug")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build mode ('Debug' or 'Release', default is 'Debug')" FORCE)
endif ()

if(GENERATOR_IS_MULTI_CONFIG)
  message(STATUS "Selected multi-config Build")
else()
  message(STATUS "Selected ${CMAKE_BUILD_TYPE} Build")
endif()

# TODO: Once available, we may want to use -fextend-lifetimes on Debug- and RelWithDebInfo builds to improve debugging experience
# https://reviews.llvm.org/D157613

string(APPEND CMAKE_CXX_FLAGS " -MP -fstack-protector-strong -ffunction-sections -fdata-sections -pipe")
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -Og -fno-omit-frame-pointer")
string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -fno-omit-frame-pointer")
string(APPEND CMAKE_CXX_FLAGS_RELEASE "")


option(CMAKE_VISIBILITY_INLINES_HIDDEN "Hide inlined functions from the DSO table (default ON)" ON)

# march=native

# NOTE: Use gcc -march=native -Q --help=target | grep -- '-march=' | cut -f3
#       to check the architecture detected by match=native
# set(PHASAR_TARGET_ARCH "" CACHE STRING "Optimize the build for the given target architecture, e.g. -march=native. Most useful in Release builds. Disabled by default")

if (DEFINED PHASAR_TARGET_ARCH)
  if (NOT CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT GENERATOR_IS_MULTI_CONFIG)
    message(WARNING "The PHASAR_TARGET_ARCH flag will be ignored in non-Release build type ${CMAKE_BUILD_TYPE}")
  else()
    set(PHASAR_TARGET_ARCH_INTERNAL "${PHASAR_TARGET_ARCH}")
  endif()
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  set(PHASAR_TARGET_ARCH_INTERNAL "native")
endif()

if (NOT "${PHASAR_TARGET_ARCH_INTERNAL}" STREQUAL "")
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-march=${PHASAR_TARGET_ARCH_INTERNAL}" MARCH_SUPPORTED)
  if (MARCH_SUPPORTED)
    message(STATUS "Target architecture '${PHASAR_TARGET_ARCH_INTERNAL}' enabled")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " -march=${PHASAR_TARGET_ARCH_INTERNAL}")
  else()
    message(WARNING "Target architecture '${PHASAR_TARGET_ARCH_INTERNAL}' not supported. Fallback to generic build")
  endif()
endif()

# Sanitizers
if (PHASAR_ENABLE_SANITIZERS)
  message(STATUS "Selected ${CMAKE_BUILD_TYPE} Build with Sanitizers")

  if(MSVC)
    set(ASAN_FLAG "/fsanitize=address")
  else()
    set(ASAN_FLAG "-fsanitize=address,undefined")
  endif()

  string(APPEND CMAKE_CXX_FLAGS " ${ASAN_FLAG}")
endif()

# LTO
if (GENERATOR_IS_MULTI_CONFIG OR CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT LTO_SUPPORTED OUTPUT LTO_SUPPORT_ERROR)

  if(LTO_SUPPORTED)
    message(STATUS "IPO/LTO enabled in Release mode")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON) # LTO
  else()
    message(STATUS "IPO/LTO not supported: ${LTO_SUPPORT_ERROR}")
  endif()
endif()

# Enable testing
enable_testing()

option(PHASAR_BUILD_UNITTESTS "Build all tests (default is ON)" ON)

option(PHASAR_BUILD_OPENSSL_TS_UNITTESTS "Build OPENSSL typestate tests (require OpenSSL, default is OFF)" OFF)

option(PHASAR_BUILD_IR "Build IR test code (default is ON)" ON)

option(PHASAR_ENABLE_CLANG_TIDY_DURING_BUILD "Run clang-tidy during build (default is OFF)" OFF)

option(PHASAR_BUILD_DOC "Build documentation" OFF)

option(PHASAR_DEBUG_LIBDEPS "Debug internal library dependencies (private linkage)" OFF)

#option(BUILD_SHARED_LIBS "Build shared libraries (default is ON)" ON)
option(PHASAR_BUILD_DYNLIB "Build one fat shared library. Requires BUILD_SHARED_LIBS to be turned OFF (default is OFF)" OFF)

if(PHASAR_BUILD_DYNLIB AND BUILD_SHARED_LIBS)
  message(FATAL_ERROR "PHASAR_BUILD_DYNLIB is incompatible with BUILD_SHARED_LIBS")
endif()

option(PHASAR_ENABLE_PIC "Build Position-Independed Code" ON)
if (PHASAR_ENABLE_PIC)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif (PHASAR_ENABLE_PIC)

# PAMM
if (NOT PHASAR_ENABLE_PAMM)
  set(PHASAR_ENABLE_PAMM "Off" CACHE STRING "Enable the performance measurement mechanism ('Off', 'Core' or 'Full', default is 'Off')" FORCE)
  set_property(CACHE PHASAR_ENABLE_PAMM PROPERTY STRINGS "Off" "Core" "Full")
endif()
if(PHASAR_ENABLE_PAMM STREQUAL "Core" AND NOT PHASAR_BUILD_UNITTESTS)
  set(PAMM_CORE ON)
  message(STATUS "PAMM metric severity level: Core")
elseif(PHASAR_ENABLE_PAMM STREQUAL "Full" AND NOT PHASAR_BUILD_UNITTESTS)
  set(PAMM_FULL ON)
  message(STATUS "PAMM metric severity level: Full")
elseif(PHASAR_BUILD_UNITTESTS AND (PHASAR_ENABLE_PAMM STREQUAL "Core" OR PHASAR_ENABLE_PAMM STREQUAL "Full"))
  message(WARNING "PAMM metric severity level: Off (due to unittests)")
else()
  message(STATUS "PAMM metric severity level: Off")
endif()

# Logger
option(PHASAR_ENABLE_DYNAMIC_LOG "Makes it possible to switch the logger on and off at runtime (default is ON)" ON)

if (PHASAR_ENABLE_DYNAMIC_LOG)
  message(STATUS "Dynamic log enabled")
  set(DYNAMIC_LOG ON)
else()
  message(STATUS "Dynamic log disabled")
endif()

# RPATH
if (NOT PHASAR_IN_TREE)
  set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})

  if (NOT "${CMAKE_INSTALL_LIBDIR}" STREQUAL "lib")
    message(STATUS "Detected CMAKE_INSTALL_LIBDIR that deviates from 'lib': ${CMAKE_INSTALL_LIBDIR}. Add ${CMAKE_INSTALL_PREFIX}/lib to the RPATH as json-schema-validator needs it")
    list(APPEND CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
  endif()

  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# Filesystem
if (LLVM_ENABLE_LIBCXX)
  set(PHASAR_STD_FILESYSTEM c++fs)
else()
  set(PHASAR_STD_FILESYSTEM stdc++fs)
endif()

# Config
set(PHASAR_CUSTOM_CONFIG_INSTALL_DIR "" CACHE STRING "If set, customizes the directory, where configuration files for PhASAR are installed (default is /usr/local/.phasar-config)")
if ("${PHASAR_CUSTOM_CONFIG_INSTALL_DIR}" STREQUAL "")
  set(PHASAR_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/.phasar-config/")
else()
  set(PHASAR_CONFIG_INSTALL_DIR "${PHASAR_CUSTOM_CONFIG_INSTALL_DIR}")
endif()

### Adding external libraries

# Threads
find_package(Threads)

# Boost
find_package(Boost 1.65.1 COMPONENTS graph REQUIRED)

# Disable clang-tidy for the external projects
set(CMAKE_CXX_CLANG_TIDY "")

# Nlohmann JSON
set(JSON_BuildTests OFF)
set(JSON_Install ON)
add_subdirectory(external/json)

# We need to work around the behavior of nlohmann_json_schema_validator and nlohmann_json here
# The validator needs the json part, but if you include it, the library of nlohmann_json_schema_validator
# is not installed, leading to linker error. But just including nlohmann_json is not sufficient, as
# in the installed state the nlohmann_json_schema_validator needs the nlohmann_json package which needs
# to be installed.
# The following workaround may collapse or become unnecessary once the issue is
# changed or fixed in nlohmann_json_schema_validator.

#Override option of nlohmann_json_schema_validator to not build its tests
set(BUILD_TESTS OFF CACHE BOOL "Build json-schema-validator-tests")

if (PHASAR_IN_TREE)
  set_property(GLOBAL APPEND PROPERTY LLVM_EXPORTS nlohmann_json_schema_validator)
endif()

# Json Schema Validator
set(JSON_VALIDATOR_INSTALL ON)
add_subdirectory(external/json-schema-validator)

# Googletest
if (NOT PHASAR_IN_TREE)
  set(BUILD_GMOCK OFF)
  set(INSTALL_GTEST OFF)
  add_subdirectory(external/googletest EXCLUDE_FROM_ALL)
  set(GTEST_INCLUDE_DIR "external/googletest/googletest/include")
else()
  # Set llvm distributed includes for gtest header
  set(GTEST_INCLUDE_DIR "${LLVM_MAIN_SRC_DIR}/utils/unittest/googletest/include")
endif()

# SQL
find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h)
find_library(SQLITE3_LIBRARY NAMES sqlite3)

option(USE_LLVM_FAT_LIB  "Link against libLLVM.so instead of the individual LLVM libraries if possible (default is OFF; always on if BUILD_SHARED_LIBS is ON)" OFF)

# LLVM
if (NOT PHASAR_IN_TREE)
  # Only search for LLVM if we build out of tree
  find_package(LLVM 14 REQUIRED CONFIG)

  find_library(LLVM_LIBRARY NAMES LLVM PATHS ${LLVM_LIBRARY_DIRS} NO_DEFAULT_PATH)

  if(USE_LLVM_FAT_LIB AND ${LLVM_LIBRARY} STREQUAL "LLVM_LIBRARY-NOTFOUND")
    message(WARNING "Did not find requested libLLVM.so. Link against individual modules instead")
    set(USE_LLVM_FAT_LIB OFF)
  elseif(BUILD_SHARED_LIBS AND NOT ${LLVM_LIBRARY} STREQUAL "LLVM_LIBRARY-NOTFOUND")
    message(STATUS "Found consolidated shared LLVM lib ${LLVM_LIBRARY} that will be linked against.")
    set(USE_LLVM_FAT_LIB ON)
  endif()
endif()

if(NOT LLVM_ENABLE_RTTI)
  message(FATAL_ERROR "PhASAR requires a LLVM version that is built with RTTI")
endif()

# Clang
option(BUILD_PHASAR_CLANG "Build the phasar_clang library (default is ON)" ON)

if(BUILD_PHASAR_CLANG)
  # The clang-cpp shared library is now the preferred way to link dynamically against libclang if we build out of tree.
  if(NOT PHASAR_IN_TREE)
    find_library(CLANG_LIBRARY NAMES clang-cpp libclang-cpp HINTS ${LLVM_LIBRARY_DIRS})
    if(${CLANG_LIBRARY} STREQUAL "CLANG_LIBRARY-NOTFOUND")
      set(NEED_LIBCLANG_COMPONENT_LIBS on)
    endif()
  endif()
  # As fallback, look for the small clang libraries
  if(PHASAR_IN_TREE OR NEED_LIBCLANG_COMPONENT_LIBS)
    set(CLANG_LIBRARY
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
      LLVMFrontendOpenMP)
  endif()

  if (PHASAR_IN_TREE)
    # Phasar needs clang headers, specificaly some that are generated by clangs table-gen
    include_directories(SYSTEM
      ${CLANG_INCLUDE_DIR}
      ${PHASAR_SRC_DIR}/../clang/include
      ${PROJECT_BINARY_DIR}/tools/clang/include
    )
  endif()
endif(BUILD_PHASAR_CLANG)

# Set up clang-tidy to run during PhASAR's compilation to indicate code smells
if (PHASAR_ENABLE_CLANG_TIDY_DURING_BUILD)
  message(STATUS "Enabled clang-tidy during build")
  set(CMAKE_CXX_CLANG_TIDY
    clang-tidy;
    -header-filter=include/phasar.*h$;
    # -warnings-as-errors=*;
  )
endif ()

# Library Dependency Headers
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(SYSTEM external/json/include/)
include_directories(SYSTEM external/json-schema-validator/src/)
include_directories(SYSTEM ${GTEST_INCLUDE_DIR})
if(NOT PHASAR_IN_TREE)
  include_directories(${LLVM_INCLUDE_DIRS})
endif()

# Library Dependency Dirs
if(NOT PHASAR_IN_TREE)
  link_directories(${LLVM_LIB_PATH} ${LLVM_LIBRARY_DIRS})
  separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
  add_definitions(${LLVM_DEFINITIONS_LIST})
  if (BUILD_PHASAR_CLANG)
    link_directories(${CLANG_LIB_PATH})
  endif()
endif()

# Installed config
configure_file(config.h.in include/phasar/Config/phasar-config.h @ONLY)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include/")

include_directories(${PHASAR_SRC_DIR}/include)

# Warnings
option(PHASAR_ENABLE_WARNINGS "Enable warnings" ON)
if (PHASAR_ENABLE_WARNINGS)
  if (MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /W4")
  else()
    string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -Wno-unused-parameter")
  endif()
endif (PHASAR_ENABLE_WARNINGS)

# Some preprocessor symbols that need to be available in phasar sources, but should not be installed
add_cxx_compile_definitions(PHASAR_SRC_DIR="${CMAKE_SOURCE_DIR}")
add_cxx_compile_definitions(PHASAR_BUILD_DIR="${CMAKE_BINARY_DIR}")

# Add PhASAR's subdirectories
add_subdirectory(lib)

# phasar-based binaries
add_subdirectory(tools)

# Swift tests
option(BUILD_SWIFT_TESTS "Builds the Swift tests (Swift compiler has to be installed manually beforehand!)" OFF)
if (BUILD_SWIFT_TESTS)
  set(CMAKE_Swift_FLAGS_RELEASE "-g")
  set(CMAKE_Swift_FLAGS_RELWITHDEBINFO "-g")
  enable_language(Swift)
endif(BUILD_SWIFT_TESTS)

# Add Phasar unittests and build all IR test code
if (PHASAR_BUILD_UNITTESTS)
  message("Phasar unittests")
  add_subdirectory(unittests)
  if(NOT PHASAR_BUILD_IR)
    message(WARNING "Set PHASAR_BUILD_IR=ON, because PHASAR_BUILD_UNITTESTS is ON")
    set(PHASAR_BUILD_IR ON)
  endif()
endif()

# Build all IR test code
if (PHASAR_BUILD_IR)
  message("Building IR test code")
  add_subdirectory(test)
endif()

set(INCLUDE_INSTALL_DIR include/ CACHE PATH "Install dir of headers")

# Install targets of phasar-cli, other executables, and libraries are to be
# found in the individual subdirectories of tools/

# Install Phasar include directory
install(DIRECTORY include/
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.def"
  PATTERN "*.h"
)

# Install the config file
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/phasar/Config/
  DESTINATION include/phasar/Config
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
  FILES_MATCHING
  PATTERN "CodeGen" EXCLUDE # CodeGen does not contain files to install
  PATTERN "phasar-*"
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
              GROUP_EXECUTE GROUP_READ
              WORLD_EXECUTE WORLD_READ
)

# Install the Phasar config files into CMAKE_INSTALL_PREFIX/.phasar-config/
install(DIRECTORY config/
  DESTINATION ${PHASAR_CONFIG_INSTALL_DIR}
  PATTERN "config/*"
  PERMISSIONS OWNER_WRITE OWNER_READ
              GROUP_WRITE GROUP_READ
              WORLD_READ
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
