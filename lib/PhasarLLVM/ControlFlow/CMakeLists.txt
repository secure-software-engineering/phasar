file(GLOB_RECURSE CONTROLFLOW_SRC *.h *.cpp)

add_phasar_library(phasar_controlflow
	${CONTROLFLOW_SRC}
)

target_link_libraries(phasar_controlflow
  
)

set_target_properties(phasar_controlflow
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX ""
)
