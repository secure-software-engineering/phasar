file(GLOB_RECURSE PHASARCLANG_SRC *.h *.cpp)

include_directories(${CLANG_INCLUDE_DIRS})

set(PHASAR_LINK_LIBS
  phasar_utils
  phasar_taintconfig
)

set(LLVM_LINK_COMPONENTS
  Support
  Core
  Option
)

add_phasar_library(phasar_clang
  ${PHASARCLANG_SRC}
)

target_link_libraries(phasar_clang
  LINK_PUBLIC
  ${CLANG_LIBRARY}
)

set_target_properties(phasar_clang
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
