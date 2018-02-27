file(GLOB_RECURSE POINTER_SRC *.h *.cpp)

# Handle the library files
add_phasar_library(phasar_pointer
	${POINTER_SRC}
)

target_link_libraries(phasar_pointer

)

set_target_properties(phasar_pointer
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX ""
)
