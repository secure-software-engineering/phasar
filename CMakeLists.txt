cmake_minimum_required (VERSION 3.14)

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

# NOTE: When we require cmake >= 3.21, we can use PROJECT_IS_TOP_LEVEL instead
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(PHASAR_BUILD_OPTIONAL_TARGETS_DEFAULT ON)
else()
  set(PHASAR_BUILD_OPTIONAL_TARGETS_DEFAULT OFF)
endif()

option(PHASAR_EXPERIMENTAL_CXX20 "Build phasar in C++20 mode. This is an experimental feature" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

set_property(GLOBAL PROPERTY TARGET_MESSAGES OFF)

option(PHASAR_ENABLE_SANITIZERS "Build PhASAR with AddressSanitizer and UBSanitizer (default is OFF)" OFF)

set(PHASAR_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PHASAR_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PHASAR_SRC_DIR}/cmake")
include("phasar_macros")

if (NOT CMAKE_BUILD_TYPE AND NOT GENERATOR_IS_MULTI_CONFIG AND NOT PHASAR_IN_TREE)
  message(STATUS "No CMAKE_BUILD_TYPE specified, setting it to Debug")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build mode ('Debug' or 'Release', default is 'Debug')" FORCE)
endif ()

if(GENERATOR_IS_MULTI_CONFIG)
  message(STATUS "Selected multi-config Build")
  set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo Release CACHE STRING "Configuration types: Debug, RelWithDebInfo and Release" FORCE)
else()
  message(STATUS "Selected ${CMAKE_BUILD_TYPE} Build")
endif()

set(DEBUG_CONFIGURATIONS DEBUG CACHE INTERNAL "" FORCE)
set(RELEASE_CONFIGURATIONS RELWITHDEBINFO RELEASE CACHE INTERNAL "" FORCE)

# TODO: Once available, we may want to use -fextend-lifetimes on Debug- and RelWithDebInfo builds to improve debugging experience
# https://reviews.llvm.org/D157613

string(APPEND CMAKE_CXX_FLAGS " -MP -fstack-protector-strong -ffunction-sections -fdata-sections -pipe")
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -fno-omit-frame-pointer")
string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -fno-omit-frame-pointer")
string(APPEND CMAKE_CXX_FLAGS_RELEASE "")

option(CMAKE_VISIBILITY_INLINES_HIDDEN "Hide inlined functions from the DSO table (default ON)" ON)

include(CheckCXXCompilerFlag)

# Handle memory issues with linking
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  string(APPEND CMAKE_CXX_FLAGS_DEBUG " -gsplit-dwarf")
  string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -gsplit-dwarf")
  set(LINKER_FLAGS_SAVE ${CMAKE_EXE_LINKER_FLAGS})

  # See LLVM_USE_SPLIT_DWARF in LLVM
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gdb-index")
  cmake_policy(PUSH)
  cmake_policy(SET CMP0056 NEW)
  check_cxx_compiler_flag("" GDB_INDEX_SUPPORTED)
  cmake_policy(POP)
  set(CMAKE_EXE_LINKER_FLAGS ${LINKER_FLAGS_SAVE})

  if(GDB_INDEX_SUPPORTED)
    link_libraries(debug "-Wl,--gdb-index")
  endif()
endif()

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
option(PHASAR_ALLOW_LTO_IN_RELEASE_BUILD "Use link-time-optimization (LTO) in Release builds, if possible (default is ON)" ON)
if(PHASAR_ALLOW_LTO_IN_RELEASE_BUILD)
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
endif()

option(PHASAR_BUILD_UNITTESTS "Build all tests (default is ON)" ${PHASAR_BUILD_OPTIONAL_TARGETS_DEFAULT})

option(PHASAR_BUILD_OPENSSL_TS_UNITTESTS "Build OPENSSL typestate tests (require OpenSSL, default is OFF)" OFF)

option(PHASAR_USE_Z3 "Build the phasar_llvm_pathsensitivity library with Z3 support for constraint solving (default is OFF)" OFF)

option(PHASAR_BUILD_IR "Build IR test code (default is ON)" ${PHASAR_BUILD_OPTIONAL_TARGETS_DEFAULT})

option(PHASAR_ENABLE_CLANG_TIDY_DURING_BUILD "Run clang-tidy during build (default is OFF)" OFF)

option(PHASAR_BUILD_DOC "Build documentation" OFF)

option(PHASAR_DEBUG_LIBDEPS "Debug internal library dependencies (private linkage)" OFF)

option(PHASAR_BUILD_TOOLS "Build PhASAR-based tools (default is ON)" ${PHASAR_BUILD_OPTIONAL_TARGETS_DEFAULT})

#option(BUILD_SHARED_LIBS "Build shared libraries (default is ON)" ON)
option(PHASAR_BUILD_DYNLIB "Build one fat shared library. Requires BUILD_SHARED_LIBS to be turned OFF (default is OFF)" OFF)

if(PHASAR_BUILD_DYNLIB AND BUILD_SHARED_LIBS)
  message(FATAL_ERROR "PHASAR_BUILD_DYNLIB is incompatible with BUILD_SHARED_LIBS")
endif()

if (PHASAR_DEBUG_LIBDEPS)
  if (NOT BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON)
    message("PHASAR_DEBUG_LIBDEPS only works for shared libraries, so set BUILD_SHARED_LIBS=ON")
  endif()
  if (PHASAR_ENABLE_SANITIZERS)
    message(FATAL_ERROR "PHASAR_DEBUG_LIBDEPS is incompatible with ASAN (see https://clang.llvm.org/docs/AddressSanitizer.html#usage)")
  endif()
endif ()

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
option(PHASAR_ENABLE_DYNAMIC_LOG "Makes it possible to switch the logger on and off at runtime; otherwise, it is turned off at compile-time (default is ON)" ON)

if (PHASAR_ENABLE_DYNAMIC_LOG)
  message(STATUS "Dynamic log enabled")
  set(DYNAMIC_LOG ON)
else()
  message(STATUS "Dynamic log disabled")
endif()

if (NOT PHASAR_IN_TREE)
  # RPATH
  set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})

  if (NOT "${CMAKE_INSTALL_LIBDIR}" STREQUAL "lib")
    message(STATUS "Detected CMAKE_INSTALL_LIBDIR that deviates from 'lib': ${CMAKE_INSTALL_LIBDIR}. Add ${CMAKE_INSTALL_PREFIX}/lib to the RPATH as json-schema-validator needs it")
    list(APPEND CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
  endif()

  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

  # Export set
  set(PHASAR_DEPS_EXPORT_SET PhasarDepsExports)
else()
  # Export set
  set(PHASAR_DEPS_EXPORT_SET LLVMExports)
endif()

set(PHASAR_DEPS_INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/phasar/deps)

# Filesystem
if (LLVM_ENABLE_LIBCXX)
  set(PHASAR_STD_FILESYSTEM c++fs)
else()
  set(PHASAR_STD_FILESYSTEM stdc++fs)
endif()

# Config
set(PHASAR_CUSTOM_CONFIG_INSTALL_DIR "" CACHE STRING "If set, customizes the directory, where configuration files for PhASAR are installed (default is ${CMAKE_INSTALL_PREFIX}/.phasar-config)")
if ("${PHASAR_CUSTOM_CONFIG_INSTALL_DIR}" STREQUAL "")
  set(PHASAR_CONFIG_INSTALL_DIR ".phasar-config/")
else()
  set(PHASAR_CONFIG_INSTALL_DIR "${PHASAR_CUSTOM_CONFIG_INSTALL_DIR}")
endif()


# Headers

add_library(phasar_interface INTERFACE)
target_include_directories(phasar_interface
  INTERFACE
    $<BUILD_INTERFACE:${PHASAR_SRC_DIR}/include/>     # The regular include folder
    $<BUILD_INTERFACE:${PHASAR_BINARY_DIR}/include/>  # The location of phasar-config.h
    $<INSTALL_INTERFACE:include/>                     # The installed include folder
)

### Adding external libraries

# Threads
find_package(Threads)

# Boost
find_package(Boost 1.65.1 COMPONENTS graph REQUIRED)

# Disable clang-tidy for the external projects
set(CMAKE_CXX_CLANG_TIDY "")

# Nlohmann JSON

include(add_nlohmann_json)
add_nlohmann_json()
add_json_schema_validator()

# Googletest
if (NOT PHASAR_IN_TREE)
  if(PHASAR_BUILD_UNITTESTS AND NOT TARGET gtest)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.13.0
    )
    FetchContent_MakeAvailable(googletest)
  endif()
else()
  # Set llvm distributed includes for gtest header
  set(GTEST_INCLUDE_DIR "${LLVM_MAIN_SRC_DIR}/utils/unittest/googletest/include")
endif()

# SQL
find_package(SQLite3)
if(SQLite3_FOUND)
  set(PHASAR_HAS_SQLITE ON)
else()
  set(PHASAR_HAS_SQLITE OFF)
endif()

option(USE_LLVM_FAT_LIB  "Link against libLLVM.so instead of the individual LLVM libraries if possible (default is OFF; always on if BUILD_SHARED_LIBS is ON)" OFF)

# LLVM
include(add_llvm)
add_llvm()

# Z3 Solver
if(PHASAR_IN_TREE)
  set (PHASAR_USE_Z3 OFF)
endif()
if(PHASAR_USE_Z3 AND NOT PHASAR_IN_TREE)
  # This z3-version is the same version LLVM requires; however, we cannot just use Z3 via the LLVM interface
  # as it lacks some functionality (such as z3::expr::simplify()) that we require
  find_package(Z3 4.7.1 REQUIRED)

  if(NOT TARGET z3)
    add_library(z3 IMPORTED SHARED)
    set_property(TARGET z3 PROPERTY
             IMPORTED_LOCATION ${Z3_LIBRARIES})
    set_property(TARGET z3 PROPERTY
             INTERFACE_INCLUDE_DIRECTORIES ${Z3_INCLUDE_DIR})
  endif()
endif()

# Clang
option(BUILD_PHASAR_CLANG "Build the phasar_clang library (default is ON)" ON)
if(BUILD_PHASAR_CLANG)
  add_clang()
endif()

# Set up clang-tidy to run during PhASAR's compilation to indicate code smells
if (PHASAR_ENABLE_CLANG_TIDY_DURING_BUILD)
  message(STATUS "Enabled clang-tidy during build")
  set(CMAKE_CXX_CLANG_TIDY
    clang-tidy;
    -header-filter=include/phasar.*h$;
    # -warnings-as-errors=*;
  )
endif ()

# Library Dependency Dirs
if(NOT PHASAR_IN_TREE)
  separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
  add_definitions(${LLVM_DEFINITIONS_LIST})
endif()

# Installed config
configure_file(config.h.in include/phasar/Config/phasar-config.h @ONLY)

# Warnings
option(PHASAR_ENABLE_WARNINGS "Enable warnings" ${PHASAR_BUILD_OPTIONAL_TARGETS_DEFAULT})
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
if(PHASAR_BUILD_TOOLS)
  add_subdirectory(tools)
endif()

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

  enable_testing()
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

set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}" CACHE PATH "Install dir of headers")
set(LIBRARY_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}" CACHE PATH "Install dir of libraries")

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

if(NOT PHASAR_IN_TREE)
  install(TARGETS phasar_interface
    EXPORT PhasarExports
  )

  # Install the export-set containing all the phasar targets
  install(EXPORT PhasarExports
    FILE PhasarExports.cmake
    NAMESPACE phasar::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/phasar"
  )
  install(EXPORT ${PHASAR_DEPS_EXPORT_SET}
    FILE ${PHASAR_DEPS_EXPORT_SET}.cmake
    NAMESPACE phasar::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/phasar"
  )
else()
  install(TARGETS phasar_interface
    EXPORT LLVMExports
  )
  set_property(GLOBAL APPEND PROPERTY LLVM_EXPORTS phasar_interface)
endif()

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
  PATH_VARS INCLUDE_INSTALL_DIR LIBRARY_INSTALL_DIR
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/phasarConfigVersion.cmake
  VERSION 2403
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
  if(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
    include(InstallRequiredSystemLibraries)
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
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_SECTION "kde")
    set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
    set(CPACK_COMPONENTS_ALL Libraries ApplicationData)
    include(CPack)
  endif(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
endif()

# Setup the doxygen code documentation
if(PHASAR_BUILD_DOC)
  find_package(Doxygen REQUIRED)

  set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
  set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

  add_custom_target(doc_doxygen ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
  )
endif()
