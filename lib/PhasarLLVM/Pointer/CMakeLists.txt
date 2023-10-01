file(GLOB_RECURSE POINTER_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_utils
  phasar_db
  phasar_pointer
  phasar_llvm_utils
  phasar_llvm_db
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  Analysis
  Passes
  Demangle
)

# Handle the library files
add_phasar_library(phasar_llvm_pointer
  ${POINTER_SRC}
)

target_link_libraries(phasar_llvm_pointer
  LINK_PRIVATE
  ${Boost_LIBRARIES}
)

set_target_properties(phasar_llvm_pointer
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
