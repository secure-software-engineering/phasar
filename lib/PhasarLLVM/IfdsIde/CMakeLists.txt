file(GLOB_RECURSE IFDSIDE_SRC *.h *.cpp)

if(BUILD_SHARED_LIBS)
	add_phasar_library(phasar_ifdside
		SHARED
		${IFDSIDE_SRC}
	)
else()
	add_phasar_library(phasar_ifdside
		STATIC
		${IFDSIDE_SRC}
	)
endif()

target_link_libraries(phasar_ifdside
  phasar_config
  phasar_utils
  phasar_pointer
  phasar_controlflow
  phasar_phasarllvm_utils

  LLVMCore
)

set_target_properties(phasar_ifdside
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
