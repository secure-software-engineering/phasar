file(GLOB_RECURSE MONO_SRC *.h *.cpp)

add_phasar_library(phasar_mono
	SHARED
	${MONO_SRC}
)

target_link_libraries(phasar_mono
  
)

set_target_properties(phasar_mono
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
