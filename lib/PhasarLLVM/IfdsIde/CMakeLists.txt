file(GLOB_RECURSE IFDSIDE_SRC *.h *.cpp)

add_phasar_library(phasar_ifdside
	SHARED
	${IFDSIDE_SRC}
)

target_link_libraries(phasar_ifdside
  
)

set_target_properties(phasar_ifdside
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
