file(GLOB_RECURSE EXPERIMENTAL_SRC *.h *.cpp)

add_phasar_library(phasar_experimental
	SHARED
	${EXPERIMENTAL_SRC}
)

target_link_libraries(phasar_experimental
  
)

set_target_properties(phasar_experimental
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
