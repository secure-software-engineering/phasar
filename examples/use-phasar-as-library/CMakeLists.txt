cmake_minimum_required(VERSION 3.12)

project(PhasarExttoolTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Build a small test tool to show how phasar may be used
add_executable(myphasartool
  myphasartool.cpp
)

find_package(phasar COMPONENTS ifdside REQUIRED)
include_directories(${PHASAR_INCLUDE_DIR})
link_directories(${PHASAR_LIBRARY_DIR})

phasar_config(myphasartool)


install(TARGETS myphasartool
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
