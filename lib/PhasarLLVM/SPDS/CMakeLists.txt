file(GLOB_RECURSE SPDS_SRC *.h *.cpp)

if(BUILD_SHARED_LIBS)
	add_phasar_library(phasar_spds
		SHARED
		${SPDS_SRC}
	)
else()
	add_phasar_library(phasar_spds
		STATIC
		${SPDS_SRC}
	)
endif()

target_link_libraries(phasar_spds
	phasar_controlflow
	phasar_wpds
)

set_target_properties(phasar_spds
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)