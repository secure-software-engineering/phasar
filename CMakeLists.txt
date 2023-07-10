cmake_minimum_required (VERSION 3.21)
project(phasar 
    VERSION 1.0.0
    HOMEPAGE_URL "https://github.com/secure-software-engineering/phasar"
    LANGUAGES C CXX)

include(cmake/options.cmake)

if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
elseif(${CMAKE_CXX_STANDARD} LESS 17)
  message(FATAL_ERROR "phasar requires CMAKE_CXX_STANDARD >= 17")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/just-simple.cmake)

set(PHASAR_CUSTOM_CONFIG_INSTALL_DIR "" CACHE STRING "If set, customizes the directory, where configuration files for PhASAR are installed (default is ${CMAKE_INSTALL_PREFIX}/.phasar-config)")
if (PHASAR_CUSTOM_CONFIG_INSTALL_DIR)
    set(PHASAR_CONFIG_INSTALL_DIR "${PHASAR_CUSTOM_CONFIG_INSTALL_DIR}")
else()
    set(PHASAR_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/.phasar-config/")
endif()
add_compile_definitions(PHASAR_CONFIG_DIR="${PHASAR_CONFIG_INSTALL_DIR}")

set(PHASAR_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
add_compile_definitions(PHASAR_DIR="${PHASAR_SRC_DIR}")

just_limit_jobs(COMPILE 1200 LINK 1200)
add_subdirectory(phasar/clang)
add_subdirectory(phasar/config)
add_subdirectory(phasar/controller)
add_subdirectory(phasar/db)
add_subdirectory(phasar/example-tool)
add_subdirectory(phasar/llvm)
add_subdirectory(phasar/utils)
add_subdirectory(phasar/test-utils)
add_subdirectory(phasar/analysisstrategy)
add_subdirectory(phasar/controlflow)
add_subdirectory(phasar/dataflow)
add_subdirectory(phasar/domain)
add_subdirectory(phasar/pointer)
add_subdirectory(phasar/typehierarchy)
add_subdirectory(phasar/cli)
