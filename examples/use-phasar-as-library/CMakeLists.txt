cmake_minimum_required(VERSION 3.12)

project(PhasarExttoolTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(phasar COMPONENTS llvm_ifdside REQUIRED)

# Or without specifying components
#   find_package(phasar REQUIRED)

# Build a small test tool to show how phasar may be used
add_executable(myphasartool
  myphasartool.cpp
)

# Old way using phasar_config:
#   phasar_config(myphasartool)

# New way using target_link_libraries:
target_link_libraries(myphasartool phasar::llvm_ifdside)

# If find_package did not specify components:
#   target_link_libraries(myphasartool phasar::phasar)
# alternatively using the default target:
#   target_link_libraries(myphasartool phasar)

install(TARGETS myphasartool
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
