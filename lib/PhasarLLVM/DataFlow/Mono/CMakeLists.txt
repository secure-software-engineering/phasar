file(GLOB_RECURSE MONO_SRC *.h *.cpp)

add_phasar_library(phasar_mono
  ${MONO_SRC}

  LINKS
    phasar_config
    phasar_utils
    phasar_llvm_utils
    phasar_llvm_controlflow
    phasar_taintconfig

  LLVM_LINK_COMPONENTS
    Core
    Support
)
