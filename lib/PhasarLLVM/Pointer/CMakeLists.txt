file(GLOB_RECURSE POINTER_SRC *.h *.cpp)

# Handle the library files
if(BUILD_SHARED_LIBS)
	add_phasar_library(phasar_pointer
		SHARED
		${POINTER_SRC}
	)
else()
	add_phasar_library(phasar_pointer
		STATIC
		${POINTER_SRC}
	)
endif()

target_link_libraries(phasar_pointer
  phasar_utils

  boost_log

  LLVMCore
  LLVMSupport
  LLVMAnalysis
)

set_target_properties(phasar_pointer
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
