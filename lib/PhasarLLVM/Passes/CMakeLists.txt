file(GLOB_RECURSE PASSES_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  Analysis
  Demangle
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_passes
    SHARED
    ${PASSES_SRC}
  )
else()
  add_phasar_library(phasar_passes
    STATIC
    ${PASSES_SRC}
  )
endif()

target_link_libraries(phasar_passes
  LINK_PUBLIC
)

set_target_properties(phasar_passes
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
