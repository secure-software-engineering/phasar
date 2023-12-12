file(GLOB_RECURSE TYPEHIERARCHY_SRC *.h *.cpp)

# Handle the library files
add_phasar_library(phasar_llvm_typehierarchy
  ${TYPEHIERARCHY_SRC}

  LINKS
    phasar_utils

  LLVM_LINK_COMPONENTS
    Core
    Demangle
    Support
    Analysis

  LINK_PUBLIC
    ${Boost_LIBRARIES}
)
