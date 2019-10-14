file(GLOB_RECURSE PLUGINS_SRC *.h *.cpp)
file(GLOB_RECURSE PLUGINS_SO *.cxx)

set(PHASAR_LINK_LIBS
  phasar_ifdside
  phasar_phasarllvm_utils
  phasar_utils
)

set(LLVM_LINK_COMPONENTS
  Core
  Support
)

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
  LINK_PUBLIC
  boost_filesystem
  boost_log
  boost_program_options
  boost_system
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
    LLVMSupport
    )
endforeach()

set_target_properties(phasar_plugins
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX "lib"
)
