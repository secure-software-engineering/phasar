file(GLOB_RECURSE EXPERIMENTAL_SRC *.h *.cpp)

if(BUILD_SHARED_LIBS)
	add_phasar_library(phasar_experimental
		SHARED
		${EXPERIMENTAL_SRC}
	)
else()
	add_phasar_library(phasar_experimental
		STATIC
		${EXPERIMENTAL_SRC}
	)
endif()

target_link_libraries(phasar_experimental
  
)

set_target_properties(phasar_experimental
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
