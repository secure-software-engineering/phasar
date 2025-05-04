file(GLOB_RECURSE POINTER_SRC *.h *.cpp)

add_phasar_library(phasar_llvm_pointer
  ${POINTER_SRC}

  LINKS
    phasar_utils
    phasar_pointer
    phasar_llvm_utils
    phasar_llvm_db

  LLVM_LINK_COMPONENTS
    Core
    Support
    Analysis
    Passes
    Demangle

  LINK_PRIVATE
    ${Boost_LIBRARIES}
)
