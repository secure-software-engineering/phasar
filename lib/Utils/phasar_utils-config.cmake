set(PHASAR_utils_COMPONENT_FOUND 1)

find_package(Boost COMPONENTS log REQUIRED)

list(APPEND
  PHASAR_LLVM_DEPS
  Support
  Core
  BitWriter)

list(APPEND
  PHASAR_UTILS_DEPS
  config)

foreach(dep ${PHASAR_UTILS_DEPS})
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
  phasar::phasar_utils
)

