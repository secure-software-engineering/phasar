file(GLOB_RECURSE IFDSIDE_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_config
  phasar_utils
  phasar_llvm_pointer
  phasar_llvm
  phasar_llvm_typehierarchy
  phasar_llvm_controlflow
  phasar_phasarllvm_utils
  phasar_db
  phasar_taintconfig
  ${Boost_LIBRARIES}
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  Demangle
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_llvm_ifdside
    SHARED
    ${IFDSIDE_SRC}
  )
else()
  add_phasar_library(phasar_llvm_ifdside
    STATIC
    ${IFDSIDE_SRC}
  )
endif()

set_target_properties(phasar_llvm_ifdside
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
