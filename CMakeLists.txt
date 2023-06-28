cmake_minimum_required (VERSION 3.0)

# Check if we build within the llvm source tree
if (DEFINED LLVM_MAIN_SRC_DIR)
  set(PHASAR_IN_TREE 1)
endif()

if (NOT PHASAR_IN_TREE)
  project (phasar)
  set(CMAKE_PROJECT_NAME "phasar")
endif ()

option(PHASAR_EXPERIMENTAL_CXX20 "Build phasar in C++20 mode. THis is an experimental feature" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
if(PHASAR_EXPERIMENTAL_CXX20)
  message(STATUS "Selected experimental C++ build")
  set(CMAKE_CXX_STANDARD 20)
else()
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

set_property(GLOBAL PROPERTY TARGET_MESSAGES OFF)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build mode ('DebugSan' or 'Debug' or 'Release', default is 'Debug')" FORCE)
endif ()

if(CMAKE_BUILD_TYPE STREQUAL "DebugSan")
  message(STATUS "Selected Debug Build with sanitizers")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP -fvisibility-inlines-hidden -fstack-protector-strong -ffunction-sections -fdata-sections -pipe -g -fno-omit-frame-pointer -fsanitize=address,undefined")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Selected Debug Build")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP -fvisibility-inlines-hidden -fstack-protector-strong -ffunction-sections -fdata-sections -pipe -g")
else()
  message(STATUS "Selected Release Build")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP -fvisibility-inlines-hidden -fstack-protector-strong -ffunction-sections -fdata-sections -pipe")
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

option(PHASAR_BUILD_UNITTESTS "Build all tests (default is ON)" ON)

option(BUILD_SWIFT_TESTS "Builds the Swift tests (Swift compiler has to be installed manually beforehand!)" OFF)
if (BUILD_SWIFT_TESTS)
  set(CMAKE_Swift_FLAGS_RELEASE "-g")
  set(CMAKE_Swift_FLAGS_RELWITHDEBINFO "-g")
  enable_language(Swift)
endif(BUILD_SWIFT_TESTS)

option(PHASAR_BUILD_OPENSSL_TS_UNITTESTS "Build OPENSSL typestate tests (require OpenSSL, default is OFF)" OFF)

option(PHASAR_BUILD_IR "Build IR test code (default is ON)" ON)

option(PHASAR_ENABLE_CLANG_TIDY_DURING_BUILD "Run clang-tidy during build (default is OFF)" OFF)

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

option(PHASAR_ENABLE_DYNAMIC_LOG "Makes it possible to switch the logger on and off at runtime (default is ON)" ON)

if (PHASAR_ENABLE_DYNAMIC_LOG)
  message(STATUS "Dynamic log enabled")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDYNAMIC_LOG")
else()
  message(STATUS "Dynamic log disabled")
endif()

configure_file(config.h.in config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include_directories(
  ${PHASAR_SRC_DIR}/include
)

if (NOT PHASAR_IN_TREE)
  set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})

  if (NOT "${CMAKE_INSTALL_LIBDIR}" STREQUAL "lib")
    message(STATUS "Detected CMAKE_INSTALL_LIBDIR that deviates from 'lib': ${CMAKE_INSTALL_LIBDIR}. Add ${CMAKE_INSTALL_PREFIX}/lib to the RPATH as json-schema-validator needs it")
    list(APPEND CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
  endif()

  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

if (LLVM_ENABLE_LIBCXX)
  set(PHASAR_STD_FILESYSTEM c++fs)
else()
  set(PHASAR_STD_FILESYSTEM stdc++fs)
endif()

set(PHASAR_CUSTOM_CONFIG_INSTALL_DIR "" CACHE STRING "If set, customizes the directory, where configuration files for PhASAR are installed (default is /usr/local/.phasar-config)")
if ("${PHASAR_CUSTOM_CONFIG_INSTALL_DIR}" STREQUAL "")
  set(PHASAR_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/.phasar-config/")
else()
  set(PHASAR_CONFIG_INSTALL_DIR "${PHASAR_CUSTOM_CONFIG_INSTALL_DIR}")
endif()

add_compile_definitions(PHASAR_CONFIG_DIR="${PHASAR_CONFIG_INSTALL_DIR}")
add_compile_definitions(PHASAR_DIR="${PHASAR_SRC_DIR}")


### Adding external libraries
# Threads
find_package(Threads)

# Boost
find_package(Boost 1.65.1 COMPONENTS graph ${BOOST_THREAD} REQUIRED)
#find_package(Boost 1.72.0 COMPONENTS graph  ${BOOST_THREAD} REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# Disable clang-tidy for the external projects
set(CMAKE_CXX_CLANG_TIDY "")

# We need to work around the behavior of nlohmann_json_schema_validator and nlohmann_json here
# The validator needs the json part, but if you include it, the library of nlohmann_json_schema_validator
# is not installed, leading to linker error. But just including nlohmann_json is not sufficient, as
# in the installed state the nlohmann_json_schema_validator needs the nlohmann_json package which needs
# to be installed.
# The following workaround may collapse or become unnecessary once the issue is
# changed or fixed in nlohmann_json_schema_validator.

#Override option of nlohmann_json_schema_validator to not build its tests
set(BUILD_TESTS OFF CACHE BOOL "Build json-schema-validator-tests")

# Make nlohmann_json_schema_validator happy by telling it how to find the single include of nlohmann_json
include_directories(SYSTEM external/json/single_include/)

if (PHASAR_IN_TREE)
  set_property(GLOBAL APPEND PROPERTY LLVM_EXPORTS nlohmann_json_schema_validator)
endif()

set(nlohmann_json_DIR ${PHASAR_SRC_DIR}/external/json/single_include/)

# Json Schema Validator
set(JSON_VALIDATOR_INSTALL ON)
add_subdirectory(external/json-schema-validator)
include_directories(external/json-schema-validator/src/)

# now we finally add the subdirectory
set(JSON_BuildTests OFF)
set(JSON_Install ON)
add_subdirectory(external/json)

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

# SQL
find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h)
find_library(SQLITE3_LIBRARY NAMES sqlite3)
include_directories(${SQLITE3_INCLUDE_DIR})

# LLVM
if (NOT PHASAR_IN_TREE)
  # Only search for LLVM if we build out of tree
  find_package(LLVM 14 REQUIRED CONFIG)
  include_directories(${LLVM_INCLUDE_DIRS})
  link_directories(${LLVM_LIB_PATH} ${LLVM_LIBRARY_DIRS})
endif()

add_definitions(${LLVM_DEFINITIONS})

if (NOT PHASAR_IN_TREE)
  find_library(LLVM_LIBRARY NAMES LLVM PATHS ${LLVM_LIBRARY_DIRS} NO_DEFAULT_PATH)
  if(NOT ${LLVM_LIBRARY} STREQUAL "LLVM_LIBRARY-NOTFOUND")
    message(STATUS "Found consolidated shared LLVM lib " ${LLVM_LIBRARY} " that will be linked against.")
    set(USE_LLVM_FAT_LIB on)
  endif()
endif()

# Clang
# The clang-cpp shared library is now the preferred way to link dynamically against libclang if we build out of tree.
if(NOT PHASAR_IN_TREE)
  find_library(CLANG_LIBRARY NAMES clang-cpp libclang-cpp REQUIRED HINTS ${LLVM_LIBRARY_DIRS})
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
if (NOT PHASAR_IN_TREE)
  # Only search for clang if we build out of tree
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

# Set up clang-tidy to run during PhASAR's compilation to indicate code smells
if (PHASAR_ENABLE_CLANG_TIDY_DURING_BUILD)
  message(STATUS "Enabled clang-tidy during build")
  set(CMAKE_CXX_CLANG_TIDY
    clang-tidy;
    -header-filter=include/phasar.*h$;
    # -warnings-as-errors=*;
  )
endif ()

# Add PhASAR's subdirectories
add_subdirectory(include)
add_subdirectory(lib)

set(LLVM_LINK_COMPONENTS
  coverage
  coroutines
  demangle
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

# Install targets of phasar-cli, other executables, and libraries are to be
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
# set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost_program_options (>= 1.66.0),
#                                   libboost_graph (>= 1.66.0),
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
