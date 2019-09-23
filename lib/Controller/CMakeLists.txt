file(GLOB_RECURSE CONTROLLER_SRC *.h *.cpp)

if(BUILD_SHARED_LIBS)
	add_phasar_library(phasar_controller
		SHARED
		${CONTROLLER_SRC}
	)
else()
	add_phasar_library(phasar_controller
		STATIC
		${CONTROLLER_SRC}
	)
endif()

find_package(Boost COMPONENTS log REQUIRED)
target_link_libraries(phasar_controller
  phasar_ifdside
  phasar_mono
  phasar_plugins

  LLVMCore

  curl
  ${Boost_LIBRARIES}
)

set_target_properties(phasar_controller
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
