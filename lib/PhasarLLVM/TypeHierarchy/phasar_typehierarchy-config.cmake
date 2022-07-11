set(PHASAR_typehierarchy_COMPONENT_FOUND 1)

find_package(Boost COMPONENTS graph REQUIRED)

list(APPEND
  PHASAR_LLVM_DEPS
  Support
  Core
  Analysis)

list(APPEND
  PHASAR_TYPEHIERARCHY_DEPS
  utils
)

foreach(dep ${PHASAR_TYPEHIERARCHY_DEPS})
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
  phasar::phasar_typehierarchy
)
