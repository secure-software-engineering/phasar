file(GLOB_RECURSE CONTROLFLOW_SRC *.h *.cpp)

if(BUILD_SHARED_LIBS)
	add_phasar_library(phasar_controlflow
		SHARED
		${CONTROLFLOW_SRC}
	)
else()
	add_phasar_library(phasar_controlflow
		STATIC
		${CONTROLFLOW_SRC}
	)
endif()

target_link_libraries(phasar_controlflow
  phasar_pointer
  phasar_db
)

set_target_properties(phasar_controlflow
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
