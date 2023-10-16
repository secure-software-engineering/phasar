file(GLOB_RECURSE PHASARCLANG_SRC *.h *.cpp)



add_phasar_library(phasar_clang
  ${PHASARCLANG_SRC}
  LINKS
    phasar_utils
    phasar_taintconfig

  LLVM_LINK_COMPONENTS
    Support
    Core
    Option

  LINK_PUBLIC
    ${CLANG_LIBRARY}
)

target_include_directories(phasar_clang
  PUBLIC ${CLANG_INCLUDE_DIRS}
)
