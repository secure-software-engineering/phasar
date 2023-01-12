set(PHASAR_phasarllvm_utils_COMPONENT_FOUND 1)

list(APPEND
  PHASAR_LLVM_DEPS
  Support
  Core
  BitWriter
  Demangle
)

list(APPEND
  PHASAR_PHASARLLVM_UTILS_DEPS
  config
  utils
)

foreach(dep ${PHASAR_PHASARLLVM_UTILS_DEPS})
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
  phasar::phasar_llvm_utils
)
