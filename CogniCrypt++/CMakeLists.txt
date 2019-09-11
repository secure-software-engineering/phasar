cmake_minimum_required(VERSION 3.12)
add_subdirectory(CrySL)
set(CCPP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
  ${CCPP_SRC_DIR}/CrySL/include
  ${CCPP_SRC_DIR}/CrySL/include/Parser
  ${CCPP_SRC_DIR}/CrySL/include/FormatConverter
  
)
include_directories(SYSTEM /usr/local/include/antlr4-runtime)
find_package(Threads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lstdc++fs")

# Build a standalone parser/typechecker for CrySL
add_executable(cryslparser
    CrySL/main.cpp
)
link_directories(/usr/local/lib)

target_link_libraries(cryslparser
    antlr4-runtime
    pthread
    Parser
)