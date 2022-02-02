file(GLOB_RECURSE CONTROLLER_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_ifdside
  phasar_mono
  phasar_plugins
  phasar_db
  phasar_pointer
  phasar_typehierarchy
  phasar_controlflow
  phasar_phasarllvm_utils
  phasar_utils
  phasar_analysis_strategy
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
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

find_package(Boost COMPONENTS log REQUIRED)
target_link_libraries(phasar_controller
  LINK_PUBLIC
  curl
  ${Boost_LIBRARIES}
  ${PHASAR_STD_FILESYSTEM}
)

set_target_properties(phasar_controller
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
