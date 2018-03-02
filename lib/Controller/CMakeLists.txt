file(GLOB_RECURSE CONTROLLER_SRC *.h *.cpp)

add_phasar_library(phasar_controller
	SHARED
	${CONTROLLER_SRC}
)

target_link_libraries(phasar_controller
	LINK_PUBLIC
)

set_target_properties(phasar_controller
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
