file(GLOB_RECURSE UTILS_SRC *.h *.cpp)

if(PHASAR_ENABLE_PAMM STREQUAL "Off" AND NOT PHASAR_BUILD_UNITTESTS)
  message("Not compiling PAMM.cpp since PAMM and Unittests are disabled.")
  get_filename_component(pamm_src PAMM.cpp ABSOLUTE)
  list(REMOVE_ITEM UTILS_SRC ${pamm_src})
endif()

add_phasar_library(phasar_utils
  ${UTILS_SRC}

  LLVM_LINK_COMPONENTS
    Core
    Support
    BitWriter
    Demangle

  LINK_PRIVATE
    ${PHASAR_STD_FILESYSTEM}
  LINK_PUBLIC
    nlohmann_json::nlohmann_json
)

set_target_properties(phasar_utils
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
