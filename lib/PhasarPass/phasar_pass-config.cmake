set(PHASAR_pass_COMPONENT_FOUND 1)

find_package(Boost COMPONENTS graph program_options REQUIRED)

list(APPEND
  PHASAR_LLVM_DEPS
  Support
  Core)

list(APPEND
  PHASAR_PASS_DEPS
  config
  controlflow
  db
  ifdside
  mono
  passes
  phasarllvm_utils
  pointer
  typehierarchy
  utils
)

foreach(dep ${PHASAR_PASS_DEPS})
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
  phasar::phasar_pass
)
