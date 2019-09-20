file(GLOB_RECURSE EXPERIMENTAL_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
)

set(LLVM_LINK_COMPONENTS
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_experimental
    SHARED
    ${EXPERIMENTAL_SRC}
  )
else()
  add_phasar_library(phasar_experimental
    STATIC
    ${EXPERIMENTAL_SRC}
  )
endif()

set_target_properties(phasar_experimental
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
