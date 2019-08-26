file(GLOB_RECURSE MONO_SRC *.h *.cpp)

if(BUILD_SHARED_LIBS)
	add_phasar_library(phasar_mono
		SHARED
		${MONO_SRC}
	)
else()
	add_phasar_library(phasar_mono
		STATIC
		${MONO_SRC}
	)
endif()

target_link_libraries(phasar_mono
  phasar_config
  phasar_utils
  phasar_phasarllvm_utils

  LLVMCore
)

set_target_properties(phasar_mono
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
