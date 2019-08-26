file(GLOB_RECURSE PLUGINS_SRC *.h *.cpp)
file(GLOB_RECURSE PLUGINS_SO *.cxx)

# Handle the library files
if(BUILD_SHARED_LIBS)
	add_phasar_library(phasar_plugins
		SHARED
		${PLUGINS_SRC}
	)
else()
	add_phasar_library(phasar_plugins
		STATIC
		${PLUGINS_SRC}
	)
endif()

target_link_libraries(phasar_plugins
  phasar_ifdside
  boost_system
  boost_filesystem
  boost_log
  ${CMAKE_DL_LIBS}
)

# Handle all plugins
foreach(plugin ${PLUGINS_SO})
  get_filename_component(plugin_name ${plugin} NAME_WE)
  add_library(${plugin_name} SHARED ${plugin})
  set_target_properties(${plugin_name} PROPERTIES PREFIX "")
  target_link_libraries(${plugin_name}
    phasar_plugins
    phasar_config
    phasar_utils
    phasar_ifdside
    LLVMCore
    )
endforeach()

set_target_properties(phasar_plugins
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
