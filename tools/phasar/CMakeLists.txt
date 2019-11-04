# Build a stand-alone executable
if(PHASAR_IN_TREE)
  add_phasar_executable(phasar
    phasar.cpp
  )

  # Build a small test tool to show how phasar may be used
  add_phasar_executable(myphasartool
    myphasartool.cpp
  )

  add_phasar_executable(syncpdstest
    syncpdstest.cpp
  )

  add_phasar_executable(boomerangtest
    boomerangtest.cpp
  )

  add_phasar_executable(wpdstest
    wpdstest.cpp
  )

  add_phasar_executable(phasarae
    phasarae.cpp
  )
else()
  add_executable(phasar
    phasar.cpp
  )

  # Build a small test tool to show how phasar may be used
  add_executable(myphasartool
    myphasartool.cpp
  )

  add_executable(syncpdstest
    syncpdstest.cpp
  )

  add_executable(boomerangtest
    boomerangtest.cpp
  )

  add_executable(wpdstest
    wpdstest.cpp
  )

  add_executable(phasarae
    phasarae.cpp
  )
endif()

# Warning! There is a another listing of libraries inside cmake/phasar_macros.cmake.
# If this list is altered the other one should be altered accordingly.
target_link_libraries(phasar
  LINK_PUBLIC
  phasar_config
  phasar_controller
  phasar_controlflow
  phasar_phasarllvm_utils
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
target_link_libraries(phasar
  LINK_PUBLIC
  ${llvm_libs}
)
endif()

set(LLVM_LINK_COMPONENTS
)

target_link_libraries(myphasartool
  LINK_PUBLIC
  phasar_config
  phasar_controller
  phasar_db
  phasar_experimental
  phasar_clang
  phasar_controlflow
  phasar_ifdside
  phasar_mono
  phasar_passes
  ${PHASAR_PLUGINS_LIB}
  phasar_pointer
  phasar_typehierarchy
  phasar_phasarllvm_utils
  phasar_utils
  boost_program_options
  boost_filesystem
  boost_graph
  boost_system
  boost_log
  ${BOOST_THREAD}
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
  ${CLANG_LIBRARIES}
  ${llvm_libs}
  gtest
)

target_link_libraries(wpdstest
  LINK_PUBLIC
  phasar_config
  phasar_controller
  phasar_db
  phasar_controlflow
  phasar_ifdside
  phasar_mono
  phasar_wpds
  wali
  phasar_passes
  ${PHASAR_PLUGINS_LIB}
  phasar_pointer
  phasar_typehierarchy
  phasar_phasarllvm_utils
  phasar_utils
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
  ${llvm_libs}
)

target_link_libraries(syncpdstest
  LINK_PUBLIC
  phasar_config
  phasar_controller
  phasar_db
  phasar_controlflow
  phasar_ifdside
  phasar_mono
  phasar_wpds
  phasar_syncpds
  ${PHASAR_PLUGINS_LIB}
  wali
  phasar_passes
  phasar_pointer
  phasar_typehierarchy
  phasar_phasarllvm_utils
  phasar_utils
  boost_program_options
  boost_filesystem
  boost_graph
  boost_system
  boost_log
  ${BOOST_THREAD}
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
  ${llvm_libs}
)

target_link_libraries(boomerangtest
  LINK_PUBLIC
  phasar_config
  phasar_controller
  phasar_db
  phasar_controlflow
  phasar_ifdside
  phasar_mono
  phasar_wpds
  phasar_syncpds
  ${PHASAR_PLUGINS_LIB}
  wali
  phasar_passes
  phasar_pointer
  phasar_typehierarchy
  phasar_phasarllvm_utils
  phasar_utils
  boost_program_options
  boost_filesystem
  boost_graph
  boost_system
  boost_log
  ${BOOST_THREAD}
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
  ${llvm_libs}
)

target_link_libraries(phasarae
  LINK_PUBLIC
  phasar_config
  phasar_controller
  phasar_db
  phasar_controlflow
  phasar_ifdside
  phasar_mono
  phasar_wpds
  phasar_syncpds
  ${PHASAR_PLUGINS_LIB}
  wali
  phasar_passes
  phasar_pointer
  phasar_typehierarchy
  phasar_phasarllvm_utils
  phasar_utils
  boost_program_options
  boost_filesystem
  boost_graph
  boost_system
  boost_log
  ${BOOST_THREAD}
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
  ${llvm_libs}
)
