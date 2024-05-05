file(GLOB_RECURSE POINTER_SRC *.h *.cpp)

# Handle the library files
add_phasar_library(phasar_pointer
  ${POINTER_SRC}

  LINKS
    phasar_utils

  LLVM_LINK_COMPONENTS
    Support
)
