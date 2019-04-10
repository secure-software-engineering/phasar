file(GLOB_RECURSE SYNCPDS_SRC *.h *.cpp)

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

target_link_libraries(phasar_syncpds
	phasar_controlflow
	phasar_wpds
)

set_target_properties(phasar_syncpds
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)