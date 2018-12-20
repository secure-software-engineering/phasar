file(GLOB_RECURSE PHASARPASS_SRC *.h *.cpp)

add_phasar_library(phasar_pass
	SHARED
	${PHASARPASS_SRC}
)

target_link_libraries(phasar_pass
# TODO add required libs
)

set_target_properties(phasar_pass
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
