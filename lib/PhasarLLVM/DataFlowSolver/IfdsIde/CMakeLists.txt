file(GLOB_RECURSE IFDSIDE_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_config
  phasar_utils
  phasar_pointer
  phasar_typehierarchy
  phasar_controlflow
  phasar_phasarllvm_utils
  phasar_db
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_ifdside
    SHARED
    ${IFDSIDE_SRC}
  )
else()
  add_phasar_library(phasar_ifdside
    STATIC
    ${IFDSIDE_SRC}
  )
endif()

set_target_properties(phasar_ifdside
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
