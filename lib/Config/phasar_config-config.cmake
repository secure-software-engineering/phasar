set(PHASAR_config_COMPONENT_FOUND 1)

list(APPEND
  PHASAR_LLVM_DEPS
  Support)

find_package(Boost COMPONENTS program_options REQUIRED)
