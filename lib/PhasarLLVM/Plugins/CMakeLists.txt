file(GLOB_RECURSE PLUGINS_SRC *.h *.cpp)
# file(GLOB_RECURSE PLUGINS_SO *.h *.cxx)

# Handle the plugins
# TODO make the following a for each loop
# foreach(plugin ${PLUGINS_SO}) 
# 	get_filename_component(plugin_name ${plugin} NAME_WE)
# 	add_library(${plugin_name} SHARED ${plugin})
# 	set_target_properties(${plugin_name} PROPERTIES PREFIX "")
# endforeach()

file(GLOB PLUGIN_IFDSTABULATIONPROBLEMTEST_SRC IFDSTabulationProblemTestPlugin.h IFDSTabulationProblemTestPlugin.cxx)
add_library(IFDSTabulationProblemTestPlugin SHARED ${PLUGIN_IFDSTABULATIONPROBLEMTEST_SRC})
set_target_properties(IFDSTabulationProblemTestPlugin PROPERTIES PREFIX "")

file(GLOB PLUGIN_IFDSSIMPLETAINT_SRC IFDSSimpleTaintAnalysis.h IFDSSimpleTaintAnalysis.cxx)
add_library(IFDSSimpleTaintAnalysis SHARED ${PLUGIN_IFDSSIMPLETAINT_SRC})
set_target_properties(IFDSSimpleTaintAnalysis PROPERTIES PREFIX "")

file(GLOB PLUGIN_ICFGTEST_SRC ICFGTestPlugin.h ICFGTestPlugin.cxx)
add_library(ICFGTestPlugin SHARED ${PLUGIN_ICFGTEST_SRC})
set_target_properties(ICFGTestPlugin PROPERTIES PREFIX "")

file(GLOB PLUGIN_IFDSSFBTAINT_SRC IFDSSFB901TaintAnalysis.h IFDSSFB901TaintAnalysis.cxx)
add_library(IFDSSFB901TaintAnalysis SHARED ${PLUGIN_IFDSSFBTAINT_SRC})
set_target_properties(IFDSSFB901TaintAnalysis PROPERTIES PREFIX "")

# Handle the library files
add_phasar_library(phasar_plugins
	SHARED
	${PLUGINS_SRC}
)

target_link_libraries(phasar_plugins

)

set_target_properties(phasar_plugins
	PROPERTIES
	LINKER_LANGUAGE CXX
	PREFIX "lib"
)
