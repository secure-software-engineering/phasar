file(GLOB_RECURSE TAINT_API_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_db
  phasar_controlflow
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_taintapiconfig
    SHARED
    ${TAINT_API_SRC}
  )
else()
  add_phasar_library(phasar_taintapiconfig
    STATIC
    ${TAINT_API_SRC}
  )
endif()

set_target_properties(phasar_taintapiconfig
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
