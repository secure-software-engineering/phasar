cmake_minimum_required (VERSION 3.21)
project(phasar 
    VERSION 1.0.0
    HOMEPAGE_URL "https://github.com/secure-software-engineering/phasar")

include(cmake/options.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(cmake/just-simple.cmake)

just_limit_jobs(COMPILE 1200 LINK 1200)
add_subdirectory(phasar/boomerang)
add_subdirectory(phasar/clang)
add_subdirectory(phasar/config)
add_subdirectory(phasar/controller)
add_subdirectory(phasar/db)
add_subdirectory(phasar/example-tool)
add_subdirectory(phasar/experimental)
add_subdirectory(phasar/llvm)
add_subdirectory(phasar/utils)
add_subdirectory(phasar/test-utils)
