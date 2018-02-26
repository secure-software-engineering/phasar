file(GLOB_RECURSE CONFIG_SRC *.h *.cpp)

add_phasar_library(phasar_config
	${CONFIG_SRC}
)

target_link_libraries(phasar_config
	LINK_PUBLIC
	${Boost_LIBRARIES}
)

set_target_properties(phasar_config
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX ""
)
