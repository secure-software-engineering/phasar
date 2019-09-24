cmake_minimum_required(VERSION 3.12)

project(CogniCrypt++)

set(CCPP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CCPP_SRC_DIR}/cmake")



option(CCPP_BUILD_UNITTESTS "Build all tests (default is OFF)" OFF)

include_directories(
  ${CCPP_SRC_DIR}/CrySL/include
  ${CCPP_SRC_DIR}/CrySL/include/Parser
#  ${CCPP_SRC_DIR}/CrySL/include/FormatConverter
  ${CCPP_SRC_DIR}/../include
)
if(EXISTS /usr/local/include/antlr4-runtime)
    include_directories(SYSTEM /usr/local/include/antlr4-runtime)
    link_directories(/usr/local/lib/)
elseif(EXISTS /usr/include/antlr4-runtime)
    include_directories(SYSTEM /usr/include/antlr4-runtime)
    link_directories(/usr/lib/)
else()
    message(FATAL_ERROR "Cannot find antlr4-runtime. Please make sure, this package is installed correctly")
endif()
find_package(Threads)
# Googletest
add_subdirectory(${CCPP_SRC_DIR}/../external/googletest EXCLUDE_FROM_ALL build)
include_directories(${CCPP_SRC_DIR}/../external/googletest/googletest/include)
include_directories(${CCPP_SRC_DIR}/../external/googletest/googlemock/include)

#find_package(antlr4-runtime REQUIRED)

# Boost

find_package(Boost COMPONENTS log system REQUIRED)
include_directories(${BOOST_INCLUDE_DIR})
add_definitions(-DBOOST_LOG_DYN_LINK)

add_subdirectory(CrySL)

# Unit-tests

if(CCPP_BUILD_UNITTESTS)
    message("CogniCrypt++ Unittests")
    include("ccpp_macros")
    add_subdirectory(unittests)
endif()

# Build a standalone parser/typechecker for CrySL
add_executable(cryslparser
    CrySL/main.cpp
)
#link_directories(/usr/local/lib)

target_link_libraries(cryslparser
    Parser
    stdc++fs
    ${CMAKE_THREAD_LIBS_INIT}
)