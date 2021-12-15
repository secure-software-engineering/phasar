set(PHASAR_mono_COMPONENT_FOUND 1)

list(APPEND
  PHASAR_LLVM_DEPS
  Support
  Core)

list(APPEND
  PHASAR_MONO_DEPS
  config
  utils
  phasarllvm_utils
  db
  taintconfig
)

foreach(dep ${PHASAR_MONO_DEPS})
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
  phasar::phasar_mono
)
