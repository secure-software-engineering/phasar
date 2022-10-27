file(GLOB_RECURSE PATHSENSITIVITY_SRC *.h *.cpp)

if(NOT PHASAR_USE_Z3)
  message("Not compiling LLVMPathConstraints.cpp since Z3 is disabled.")
  get_filename_component(llvmpathconstraints_src LLVMPathConstraints.cpp ABSOLUTE)
  list(REMOVE_ITEM PATHSENSITIVITY_SRC ${llvmpathconstraints_src})
endif()

set(PHASAR_LINK_LIBS
  phasar_config
  phasar_controlflow
  phasar_utils
  phasar_phasarllvm_utils
  phasar_pointer
 # libz3
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

if(PHASAR_USE_Z3)
  target_link_libraries(phasar_pathsensitivity
    LINK_PUBLIC
    z3
  )
endif()

set_target_properties(phasar_pathsensitivity
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
