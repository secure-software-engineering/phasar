file(GLOB_RECURSE PHASARPASS_SRC *.h *.cpp)

add_phasar_library(phasar_pass
  SHARED
  ${PHASARPASS_SRC}
)

target_link_libraries(phasar_pass
  boost_filesystem
  boost_graph
  boost_log
  boost_program_options
  boost_system

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
