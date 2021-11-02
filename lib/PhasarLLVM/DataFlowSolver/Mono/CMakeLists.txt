file(GLOB_RECURSE MONO_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_config
  phasar_utils
  phasar_phasarllvm_utils
  phasar_db
  phasar_taintconfig
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_mono
    SHARED
    ${MONO_SRC}
  )
else()
  add_phasar_library(phasar_mono
    STATIC
    ${MONO_SRC}
  )
endif()

set_target_properties(phasar_mono
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
