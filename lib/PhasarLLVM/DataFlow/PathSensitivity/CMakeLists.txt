file(GLOB_RECURSE PATHSENSITIVITY_SRC *.h *.cpp)

if(NOT PHASAR_USE_Z3)
  message("Not compiling LLVMPathConstraints.cpp since Z3 is disabled.")
  # get_filename_component(LLVMPathConstraints_src LLVMPathConstraints.cpp ABSOLUTE)
  # list(REMOVE_ITEM PATHSENSITIVITY_SRC ${LLVMPathConstraints_src})

 
  get_filename_component(Z3BasedPathSensitvityManager_src Z3BasedPathSensitvityManager.cpp ABSOLUTE)
  # message("Not compiling ${Z3BasedPathSensitvityManager_src} since Z3 is disabled.")
  # list(REMOVE_ITEM PATHSENSITIVITY_SRC ${Z3BasedPathSensitvityManager_src})

  set(PATHSENSITIVITY_SRC
    PathSensitivityManagerBase.cpp
  )

  message("Remaining PathSensitivity targets to build: ${PATHSENSITIVITY_SRC}")
endif()

set(PHASAR_LINK_LIBS
  phasar_config
  phasar_controlflow
  phasar_llvm_controlflow
  phasar_utils
  phasar_llvm_utils
  phasar_pointer
  phasar_llvm_pointer
)

if(PHASAR_USE_Z3)
  list(APPEND PHASAR_LINK_LIBS z3)
endif()

set(LLVM_LINK_COMPONENTS
  Core
  Support
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_llvm_pathsensitivity
    SHARED
    ${PATHSENSITIVITY_SRC}
  )
else()
  add_phasar_library(phasar_llvm_pathsensitivity
    STATIC
    ${PATHSENSITIVITY_SRC}
  )
endif()

set_target_properties(phasar_llvm_pathsensitivity
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
