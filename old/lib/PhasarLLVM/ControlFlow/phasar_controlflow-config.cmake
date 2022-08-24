set(PHASAR_controlflow_COMPONENT_FOUND 1)

list(APPEND
  PHASAR_LLVM_DEPS
  Support
  Core)

list(APPEND
  PHASAR_CONTROLFLOW_DEPS
  pointer
  typehierarchy
  db
  utils
)

foreach(dep ${PHASAR_CONTROLFLOW_DEPS})
  list(APPEND
    PHASAR_NEEDED_LIBS
    phasar::phasar_${dep}
  )
  if(NOT (${PHASAR_${dep}_COMPONENT_FOUND}))
    find_package(phasar COMPONENTS ${dep})
  endif()
endforeach()

list(APPEND
  PHASAR_NEEDED_LIBS
  phasar::phasar_controlflow
)
