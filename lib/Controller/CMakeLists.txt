file(GLOB_RECURSE CONTROLLER_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_ifdside
  phasar_mono
  phasar_plugins
)

set(LLVM_LINK_COMPONENTS
  Core
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_controller
    SHARED
    ${CONTROLLER_SRC}
  )
else()
  add_phasar_library(phasar_controller
    STATIC
    ${CONTROLLER_SRC}
  )
endif()

target_link_libraries(phasar_controller
  LINK_PUBLIC
  curl
  boost_log
)

set_target_properties(phasar_controller
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
