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


if(PHASAR_USE_Z3)
  set(ADDITIONAL_LINK_LIBS z3)
endif()

add_phasar_library(phasar_llvm_pathsensitivity
  ${PATHSENSITIVITY_SRC}

  LINKS
    phasar_config
    phasar_controlflow
    phasar_llvm_controlflow
    phasar_utils
    phasar_llvm_utils
    phasar_pointer
    phasar_llvm_pointer

  LLVM_LINK_COMPONENTS
    Core
    Support

  LINK_PUBLIC
    nlohmann_json::nlohmann_json
    ${ADDITIONAL_LINK_LIBS}
)
