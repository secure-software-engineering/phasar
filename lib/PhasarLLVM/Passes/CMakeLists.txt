file(GLOB_RECURSE PASSES_SRC *.h *.cpp)

if(BUILD_SHARED_LIBS)
	add_phasar_library(phasar_passes
		SHARED
		${PASSES_SRC}
	)
else()
	add_phasar_library(phasar_passes
		STATIC
		${PASSES_SRC}
	)
endif()

find_package(Boost COMPONENTS log REQUIRED)

target_link_libraries(phasar_passes
  phasar_utils
  
  ${Boost_LIBRARIES}
  
  LLVMCore
  LLVMSupport
  LLVMAnalysis
)

set_target_properties(phasar_passes
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
