set(PHASAR_clang_COMPONENT_FOUND 1)

find_package(Boost COMPONENTS log REQUIRED)

list(APPEND
  PHASAR_LLVM_DEPS
  Support
  Core
  Option)

list(APPEND
  PHASAR_CLANG_DEPS
  utils)

foreach(dep ${PHASAR_CLANG_DEPS})
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
  phasar::phasar_clang
)
