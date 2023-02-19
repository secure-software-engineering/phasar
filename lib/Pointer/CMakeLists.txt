file(GLOB_RECURSE POINTER_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_utils
  phasar_db
)

set(LLVM_LINK_COMPONENTS
  Support
)

# Handle the library files
if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_pointer
    SHARED
    ${POINTER_SRC}
  )
else()
  add_phasar_library(phasar_pointer
    STATIC
    ${POINTER_SRC}
  )
endif()

set_target_properties(phasar_pointer
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
