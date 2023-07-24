cmake_minimum_required (VERSION 3.21)
project(phasar 
    VERSION 1.0.0
    HOMEPAGE_URL "https://github.com/secure-software-engineering/phasar"
    LANGUAGES C CXX)

include(cmake/options.cmake)
include(cmake/just-simple.cmake)

# disable target completion output in terminal
set_property(GLOBAL PROPERTY TARGET_MESSAGES OFF)

# handle LTO
include(CheckIPOSupported)
check_ipo_supported(RESULT LTO_SUPPORTED OUTPUT LTO_SUPPORT_ERROR)
if(LTO_SUPPORTED)
  message(STATUS "IPO/LTO enabled")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(STATUS "IPO/LTO not supported: ${LTO_SUPPORT_ERROR}")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP -fvisibility-inlines-hidden -fstack-protector-strong -ffunction-sections -fdata-sections -pipe")

# TODO: really cache?
set(PHASAR_CUSTOM_CONFIG_INSTALL_DIR "" CACHE STRING "If set, customizes the directory, where configuration files for PhASAR are installed (default is ${CMAKE_INSTALL_PREFIX}/.phasar-config)")
if (PHASAR_CUSTOM_CONFIG_INSTALL_DIR)
  set(PHASAR_CONFIG_INSTALL_DIR "${PHASAR_CUSTOM_CONFIG_INSTALL_DIR}")
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
add_subdirectory(phasar/all)
