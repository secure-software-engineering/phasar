file(GLOB_RECURSE CONFIG_SRC *.h *.cpp)

add_phasar_library(phasar_config
  ${CONFIG_SRC}
  LINKS phasar_utils
  LINK_PUBLIC ${Boost_LIBRARIES}
  LLVM_LINK_COMPONENTS Support
)
