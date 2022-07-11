file(GLOB_RECURSE PHASARPASS_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_config
  phasar_controlflow
  phasar_db
  phasar_ifdside
  phasar_mono
  phasar_passes
  phasar_phasarllvm_utils
  phasar_pointer
  phasar_typehierarchy
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_pass
    SHARED
    ${PHASARPASS_SRC}
  )
else()
  add_phasar_library(phasar_pass
    STATIC
    ${PHASARPASS_SRC}
  )
endif()

# We specifically link internal phasar libs into phasar_pass so on that the
# llvm user side only has to specify one library.

find_package(Boost COMPONENTS graph program_options REQUIRED)
target_link_libraries(phasar_pass
  LINK_PUBLIC
  ${PHASAR_LINK_LIBS}
  ${Boost_LIBRARIES}
)

set_target_properties(phasar_pass
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
