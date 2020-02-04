file(GLOB_RECURSE UTILS_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_config
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
)

# Handle the library files
if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_phasarllvm_utils
    SHARED
    ${UTILS_SRC}
  )
else()
  add_phasar_library(phasar_phasarllvm_utils
    STATIC
    ${UTILS_SRC}
  )
endif()

find_package(Boost COMPONENTS filesystem REQUIRED)
target_link_libraries(phasar_phasarllvm_utils
  LINK_PUBLIC
  ${Boost_LIBRARIES}
)

set_target_properties(phasar_phasarllvm_utils
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
