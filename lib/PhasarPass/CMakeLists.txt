file(GLOB_RECURSE PHASARPASS_SRC *.h *.cpp)

add_phasar_library(phasar_pass
  SHARED
  ${PHASARPASS_SRC}
)

find_package(Boost COMPONENTS 
  filesystem
  graph
  log
  program_options
  REQUIRED)
  
target_link_libraries(phasar_pass
 ${Boost_LIBRARIES}

  phasar_config
  phasar_controlflow
  phasar_db
  phasar_ifdside
  phasar_mono
  phasar_passes
  phasar_phasarllvm_utils
  phasar_pointer
  phasar_utils
)

set_target_properties(phasar_pass
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
