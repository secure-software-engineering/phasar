file(GLOB_RECURSE PHASARPASS_SRC *.h *.cpp)

add_phasar_library(phasar_pass
	SHARED
	${PHASARPASS_SRC}
)

target_link_libraries(phasar_pass
  boost_program_options
  boost_filesystem
  boost_graph
  boost_system
  boost_log
  #phasar_ifdside
  #phasar_controlflow
  #phasar_db

  libphasar_ifdsided.a
  libphasar_controlflowd.a
  #libphasar_controllerd.a
  libphasar_dbd.a
  libphasar_configd.a
  #libphasar_experimentald.a
  #libphasar_monod.a
  libphasar_pointerd.a
  #libphasar_pluginsd.a
  libphasar_utilsd.a
  libphasar_passesd.a
  #phasar_phasarllvm_utilsd.a
)

set_target_properties(phasar_pass
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
