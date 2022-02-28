set(PHASAR_passes_COMPONENT_FOUND 1)

find_package(Boost COMPONENTS log filesystem REQUIRED)

list(APPEND
  PHASAR_LLVM_DEPS
  Support
  Core
  Analysis)

list(APPEND
  PHASAR_PASSES_DEPS
  utils
)

foreach(dep ${PHASAR_PASSES_DEPS})
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
  phasar::phasar_passes
)
