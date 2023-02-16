file(GLOB_RECURSE UTILS_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_config
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  BitWriter
  Demangle
)

# Handle the library files
if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_llvm_utils
    SHARED
    ${UTILS_SRC}
  )
else()
  add_phasar_library(phasar_llvm_utils
    STATIC
    ${UTILS_SRC}
  )
endif()

target_link_libraries(phasar_llvm_utils
  LINK_PUBLIC
)

set_target_properties(phasar_llvm_utils
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
