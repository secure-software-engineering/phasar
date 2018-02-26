file(GLOB_RECURSE UTILS_SRC *.h *.cpp)

add_phasar_library(phasar_utils
	${UTILS_SRC}
)

target_link_libraries(phasar_utils
	LLVMCore
	LLVMSupport
	LLVMBitWriter
	phasar_config
)

set_target_properties(phasar_utils
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX ""
)
