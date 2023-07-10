cmake_minimum_required (VERSION 3.21)
project(phasar 
    VERSION 1.0.0
    HOMEPAGE_URL "https://github.com/secure-software-engineering/phasar"
    LANGUAGES C CXX)

include(cmake/options.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/just-simple.cmake)

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
