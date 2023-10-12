file(GLOB_RECURSE PHASARPASS_SRC *.h *.cpp)

# We specifically link internal phasar libs into phasar_pass so on that the
# llvm user side only has to specify one library.

add_phasar_library(phasar_pass
  ${PHASARPASS_SRC}

  LINKS
    phasar_config
    phasar_llvm_controlflow
    phasar_llvm_db
    phasar_llvm_ifdside
    phasar_mono
    phasar_passes
    phasar_llvm_utils
    phasar_llvm_pointer
    phasar_llvm_typehierarchy
    phasar_utils

  LLVM_LINK_COMPONENTS
    Core
    Support

  LINK_PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    ${PHASAR_LINK_LIBS}
    ${Boost_LIBRARIES}
)

set_target_properties(phasar_pass
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
