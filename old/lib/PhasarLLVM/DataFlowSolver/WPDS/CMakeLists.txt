file(GLOB_RECURSE WPDS_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_utils
  phasar_ifdside
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_wpds
    SHARED
    ${WPDS_SRC}
  )
else()
  add_phasar_library(phasar_wpds
    STATIC
    ${WPDS_SRC}
  )
endif()

find_package(Boost COMPONENTS program_options REQUIRED)
target_link_libraries(phasar_wpds
  LINK_PUBLIC
  ${Boost_LIBRARIES}
)

set_target_properties(phasar_wpds
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
