file(GLOB_RECURSE PATHSENSITIVITY_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_config
  phasar_utils
  phasar_phasarllvm_utils
  phasar_pointer
  libz3
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_pathsensitivity
    SHARED
    ${PATHSENSITIVITY_SRC}
  )
else()
  add_phasar_library(phasar_pathsensitivity
    STATIC
    ${PATHSENSITIVITY_SRC}
  )
endif()


set_target_properties(phasar_pathsensitivity
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
