file(GLOB_RECURSE PLUGINS_SRC *.h *.cpp)

add_phasar_library(phasar_plugins
	${PLUGINS_SRC}
)

target_link_libraries(phasar_plugins
  
)

set_target_properties(phasar_plugins
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX ""
)
