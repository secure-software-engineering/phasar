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

find_package(phasar)
include_directories(${PHASAR_INCLUDE_DIR})
link_directories(${PHASAR_LIBRARY_DIR})

target_link_libraries(myphasartool
  LINK_PUBLIC
  phasar::phasar_config
  phasar::phasar_controller
  phasar::phasar_db
  phasar::phasar_experimental
  phasar::phasar_clang
  phasar::phasar_controlflow
  phasar::phasar_ifdside
  phasar::phasar_mono
  phasar::phasar_passes
  phasar::phasar_pointer
  phasar::phasar_typehierarchy
  phasar::phasar_phasarllvm_utils
#  phasar::phasar_utils #already introduced through phasar_clang
)

install(TARGETS myphasartool
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
