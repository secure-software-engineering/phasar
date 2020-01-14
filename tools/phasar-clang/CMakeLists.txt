# Build a stand-alone executable
if(PHASAR_IN_TREE)
  add_phasar_executable(phasar-clang
    phasar-clang.cpp
  )
else()
  add_executable(phasar-clang
    phasar-clang.cpp
  )
endif()

target_link_libraries(phasar-clang
  LINK_PUBLIC
  phasar_config
  phasar_controller
  phasar_controlflow
  phasar_phasarllvm_utils
  phasar_analysis_strategy
  phasar_ifdside
  phasar_utils
  phasar_mono
  phasar_db
  phasar_experimental
  phasar_clang
  phasar_passes
  # FIXME: cmake variable ${PHASAR_PLUGINS_LIB} is empty although it should contain phasar_plugins 
  phasar_plugins
  # ${PHASAR_PLUGINS_LIB}
  phasar_pointer
  phasar_typehierarchy
  boost_program_options
  boost_filesystem
  boost_graph
  boost_system
  boost_log
  ${BOOST_THREAD}
  ${SQLITE3_LIBRARY}
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
  ${CLANG_LIBRARIES}
)

if(NOT PHASAR_IN_TREE)
target_link_libraries(phasar-clang
  LINK_PUBLIC
  ${llvm_libs}
)
endif()

set(LLVM_LINK_COMPONENTS
)
