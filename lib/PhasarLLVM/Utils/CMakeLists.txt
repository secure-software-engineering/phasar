file(GLOB_RECURSE UTILS_SRC *.h *.cpp)

# Handle the library files
add_phasar_library(phasar_phasarllvm_utils
	SHARED
	${UTILS_SRC}
)

target_link_libraries(phasar_phasarllvm_utils

)

set_target_properties(phasar_phasarllvm_utils
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
