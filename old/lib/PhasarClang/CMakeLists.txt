file(GLOB_RECURSE PHASARCLANG_SRC *.h *.cpp)

include_directories(${CLANG_INCLUDE_DIRS})

set(PHASAR_LINK_LIBS
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Support
  Core
  Option
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_clang
    SHARED
    ${PHASARCLANG_SRC}
  )
else()
  add_phasar_library(phasar_clang
    STATIC
    ${PHASARCLANG_SRC}
  )
endif()

target_link_libraries(phasar_clang
  LINK_PUBLIC
  ${CLANG_LIBRARY}
)

set_target_properties(phasar_clang
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
