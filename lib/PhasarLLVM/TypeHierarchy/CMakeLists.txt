file(GLOB_RECURSE TYPEHIERARCHY_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  Analysis
)

# Handle the library files
if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_typehierarchy
    SHARED
    ${TYPEHIERARCHY_SRC}
  )
else()
  add_phasar_library(phasar_typehierarchy
    STATIC
    ${TYPEHIERARCHY_SRC}
  )
endif()

find_package(Boost COMPONENTS log filesystem graph REQUIRED)
target_link_libraries(phasar_typehierarchy
  LINK_PUBLIC
  ${Boost_LIBRARIES}
)

set_target_properties(phasar_typehierarchy
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
