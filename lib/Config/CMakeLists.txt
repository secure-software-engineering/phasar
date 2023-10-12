file(GLOB_RECURSE CONFIG_SRC *.h *.cpp)

add_phasar_library(phasar_config
  ${CONFIG_SRC}
  LINKS phasar_utils
  LINK_PRIVATE ${Boost_LIBRARIES}
  LLVM_LINK_COMPONENTS Support
)

set_target_properties(phasar_config
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
