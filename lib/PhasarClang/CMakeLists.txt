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

if(PHASAR_IN_TREE)
  # Some phasar-clang headers depend on generated files e.g files included from clang/AST/ASTFwd.h
  # Make sure, phasar-clang is built *after* these files have been generated
  add_dependencies(phasar_clang intrinsics_gen)
endif()
