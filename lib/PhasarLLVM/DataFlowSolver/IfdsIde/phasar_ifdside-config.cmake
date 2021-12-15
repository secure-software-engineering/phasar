set(PHASAR_ifdside_COMPONENT_FOUND 1)

find_package(Boost COMPONENTS program_options REQUIRED)

list(APPEND
  LLVM_DEPS
  Support
  Core)

list(APPEND
  IFDSIDE_DEPS
  config
  utils
  pointer
  typehierarchy
  controlflow
  phasarllvm_utils
  db
  taintconfig
)

foreach(dep ${IFDSIDE_DEPS})
  list(APPEND
    NEEDED_LIBS
    phasar::phasar_${dep}
  )
  if(NOT (${PHASAR_${dep}_COMPONENT_FOUND}))
    find_package(phasar COMPONENTS ${dep})
  endif()
endforeach()

list(APPEND
  PHASAR_NEEDED_LIBS
  phasar::phasar_ifdside
)
