file(GLOB_RECURSE UTILS_SRC *.h *.cpp)

# Handle the library files
if(BUILD_SHARED_LIBS)
	add_phasar_library(phasar_phasarllvm_utils
		SHARED
		${UTILS_SRC}
	)
else()
	add_phasar_library(phasar_phasarllvm_utils
		STATIC
		${UTILS_SRC}
	)
endif()

target_link_libraries(phasar_phasarllvm_utils
  boost_filesystem
  boost_system
)

set_target_properties(phasar_phasarllvm_utils
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
