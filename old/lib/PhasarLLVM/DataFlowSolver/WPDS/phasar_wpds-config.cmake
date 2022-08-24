set(PHASAR_wpds_COMPONENT_FOUND 1)

find_package(Boost COMPONENTS program_options REQUIRED)

list(APPEND
  PHASAR_LLVM_DEPS
  Support
  Core)

list(APPEND
  PHASAR_WPDS_DEPS
  utils
  ifdside
)

foreach(dep ${PHASAR_WPDS_DEPS})
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
  phasar::phasar_wpds
)
