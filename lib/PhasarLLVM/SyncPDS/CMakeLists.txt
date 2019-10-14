file(GLOB_RECURSE SYNCPDS_SRC *.h *.cpp)

set(PHASAR_LINK_LIBS
  phasar_controlflow
  phasar_wpds
)

if(BUILD_SHARED_LIBS)
  add_phasar_library(phasar_syncpds
    SHARED
    ${SYNCPDS_SRC}
  )
else()
  add_phasar_library(phasar_syncpds
    STATIC
    ${SYNCPDS_SRC}
  )
endif()

set_target_properties(phasar_syncpds
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
