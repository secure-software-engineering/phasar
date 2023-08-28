file(GLOB_RECURSE CONTROLLER_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_llvm_ifdside
  phasar_mono
  phasar_llvm_db
  phasar_llvm_pointer
  phasar_llvm_typehierarchy
  phasar_llvm_controlflow
  phasar_llvm_utils
  phasar_utils
  phasar_analysis_strategy
  phasar_taintconfig
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  Demangle
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
  ${CMAKE_THREAD_LIBS_INIT}
  ${PHASAR_STD_FILESYSTEM}
)

set_target_properties(phasar_controller
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
