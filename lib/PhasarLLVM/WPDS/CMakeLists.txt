file(GLOB_RECURSE WPDS_SRC *.h *.cpp)

if(BUILD_SHARED_LIBS)
	add_phasar_library(phasar_wpds
		SHARED
		${WPDS_SRC}
	)
else()
	add_phasar_library(phasar_wpds
		STATIC
		${WPDS_SRC}
	)
endif()

target_link_libraries(phasar_wpds
  
)

set_target_properties(phasar_wpds
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)