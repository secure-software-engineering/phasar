set(PHASAR_pathsensitivity_COMPONENT_FOUND 1)


list(APPEND
  PHASAR_PATHSENSITIVITY_DEPS
  utils
)

foreach(dep ${PHASAR_PATHSENSITIVITY_DEPS})
  message("dsear " ${dep})
  list(APPEND
    PHASAR_NEEDED_LIBS
    phasar::phasar_${dep}
  )
  if(NOT (${PHASAR_${dep}_COMPONENT_FOUND}))
    find_package(phasar COMPONENTS ${dep})
  endif()
endforeach()

find_package(Z3 PATHS ${phasar_DIR}/../../phasar/cmake/z3 NO_DEFAULT_PATH REQUIRED)

list(APPEND
  PHASAR_NEEDED_LIBS
  phasar::phasar_pathsensitivity
)
