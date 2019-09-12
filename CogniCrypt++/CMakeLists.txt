cmake_minimum_required(VERSION 3.12)

set(CCPP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
  ${CCPP_SRC_DIR}/CrySL/include
  ${CCPP_SRC_DIR}/CrySL/include/Parser
  ${CCPP_SRC_DIR}/CrySL/include/FormatConverter
  
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
#find_package(antlr4-runtime REQUIRED)

add_subdirectory(CrySL)

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