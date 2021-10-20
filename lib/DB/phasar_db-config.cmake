set(PHASAR_db_COMPONENT_FOUND 1)

list(APPEND
  PHASAR_LLVM_DEPS
  Support
  Core
  IRReader
  Linker
  BitWriter
  Passes)

list(APPEND
  PHASAR_DB_DEPS
  passes
  utils
)

foreach(dep ${PHASAR_DB_DEPS})
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
  phasar::phasar_db
)
