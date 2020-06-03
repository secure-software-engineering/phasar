set(PHASAR_syncpds_COMPONENT_FOUND 1)

list(APPEND
  PHASAR_SYNCPDS_DEPS
  controlflow
  wpds
)

foreach(dep ${PHASAR_SYNCPDS_DEPS})
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
  phasar::phasar_syncpds
)

