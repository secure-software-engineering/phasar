file(GLOB_RECURSE CONTROLFLOW_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_pointer
  phasar_typehierarchy
  phasar_db
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_controlflow
    SHARED
    ${CONTROLFLOW_SRC}
  )
else()
  add_phasar_library(phasar_controlflow
    STATIC
    ${CONTROLFLOW_SRC}
  )
endif()

set_target_properties(phasar_controlflow
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
