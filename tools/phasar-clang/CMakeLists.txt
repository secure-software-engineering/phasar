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

find_package(Boost COMPONENTS program_options graph ${BOOST_THREAD} REQUIRED)
if(USE_LLVM_FAT_LIB)
  llvm_config(phasar-clang USE_SHARED ${LLVM_LINK_COMPONENTS})
else()
  llvm_config(phasar-clang ${LLVM_LINK_COMPONENTS})
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
  # phasar_clang
  phasar_passes
  # FIXME: cmake variable ${PHASAR_PLUGINS_LIB} is empty although it should contain phasar_plugins
  phasar_plugins
  # ${PHASAR_PLUGINS_LIB}
  phasar_pointer
  phasar_typehierarchy
  ${SQLITE3_LIBRARY}
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
  # ${CLANG_LIBRARY}
  LINK_PRIVATE
  ${PHASAR_STD_FILESYSTEM}
)

set(LLVM_LINK_COMPONENTS
)
