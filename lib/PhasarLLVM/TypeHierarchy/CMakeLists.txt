file(GLOB_RECURSE TYPEHIERARCHY_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Core
  Demangle
  Support
  Analysis
)

# Handle the library files
add_phasar_library(phasar_llvm_typehierarchy
  ${TYPEHIERARCHY_SRC}
)

target_link_libraries(phasar_llvm_typehierarchy
  LINK_PRIVATE
  ${Boost_LIBRARIES}
)

set_target_properties(phasar_llvm_typehierarchy
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
