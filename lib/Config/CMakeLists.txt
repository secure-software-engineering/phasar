file(GLOB_RECURSE CONFIG_SRC *.h *.cpp)

add_phasar_library(phasar_config
  ${CONFIG_SRC}
  LINKS phasar_utils
  LINK_PRIVATE ${Boost_LIBRARIES}
  LLVM_LINK_COMPONENTS Support
)

add_library(phasar_config_mod)
target_sources(phasar_config_mod PUBLIC
  FILE_SET cxx_modules TYPE CXX_MODULES
  FILES config.cppm
)
target_link_libraries(phasar_config_mod PUBLIC phasar_config)
