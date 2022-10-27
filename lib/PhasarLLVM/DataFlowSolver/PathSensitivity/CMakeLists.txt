file(GLOB_RECURSE PATHSENSITIVITY_SRC *.h *.cpp)

if(NOT PHASAR_USE_Z3)
message("Not compiling LLVMPathConstraints.cpp since Z3 is disabled.")
# get_filename_component(LLVMPathConstraints_src LLVMPathConstraints.cpp ABSOLUTE)
# list(REMOVE_ITEM PATHSENSITIVITY_SRC ${LLVMPathConstraints_src})


get_filename_component(Z3BasedPathSensitvityManager_src Z3BasedPathSensitvityManager.cpp ABSOLUTE)
# message("Not compiling ${Z3BasedPathSensitvityManager_src} since Z3 is disabled.")
# list(REMOVE_ITEM PATHSENSITIVITY_SRC ${Z3BasedPathSensitvityManager_src})

# TODO: For whatever reason the list REMOVE_ITEM does not work...

set(PATHSENSITIVITY_SRC
  PathSensitivityManagerBase.cpp
)

message("Remaining PathSensitivity targets to build: ${PATHSENSITIVITY_SRC}")
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
