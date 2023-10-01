file(GLOB_RECURSE CONFIG_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Support
)

add_phasar_library(phasar_config
  ${CONFIG_SRC}
)

target_link_libraries(phasar_config
  LINK_PRIVATE
  ${Boost_LIBRARIES}
)

set_target_properties(phasar_config
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
