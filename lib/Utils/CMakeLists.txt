file(GLOB_RECURSE UTILS_SRC *.h *.cpp)

if(PHASAR_ENABLE_PAMM STREQUAL "Off" AND NOT PHASAR_BUILD_UNITTESTS)
  message("Not compiling PAMM.cpp since PAMM and Unittests are disabled.")
  get_filename_component(pamm_src PAMM.cpp ABSOLUTE)
  list(REMOVE_ITEM UTILS_SRC ${pamm_src})
endif()

set(PHASAR_LINK_LIBS
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  BitWriter
  Demangle
)

add_phasar_library(phasar_utils
  ${UTILS_SRC}
)

target_include_directories(phasar_utils PUBLIC ${LLVM_INCLUDE_DIRS})

target_link_libraries(phasar_utils
  LINK_PUBLIC
  ${PHASAR_STD_FILESYSTEM}
)

set_target_properties(phasar_utils
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
