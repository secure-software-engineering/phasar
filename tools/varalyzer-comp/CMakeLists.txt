# Build a stand-alone executable
if(PHASAR_IN_TREE)
  add_phasar_executable(varalyzer-comp
    varalyzer-comp.cpp
  )
else()
  add_executable(varalyzer-comp
    varalyzer-comp.cpp
  )
endif()

find_package(Boost COMPONENTS log filesystem program_options graph ${BOOST_THREAD} REQUIRED)
# Warning! There is a another listing of libraries inside cmake/phasar_macros.cmake.
# If this list is altered the other one should be altered accordingly.
target_link_libraries(varalyzer-comp
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
  phasar_passes
  phasar_varalyzer_utils
  # FIXME: cmake variable ${PHASAR_PLUGINS_LIB} is empty although it should contain phasar_plugins 
  phasar_plugins
  # ${PHASAR_PLUGINS_LIB}
  phasar_pointer
  phasar_typehierarchy
  ${SQLITE3_LIBRARY}
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
)

if(USE_LLVM_FAT_LIB)
  llvm_config(varalyzer-comp USE_SHARED ${LLVM_LINK_COMPONENTS})
else()
  llvm_config(varalyzer-comp ${LLVM_LINK_COMPONENTS})
endif()

set(LLVM_LINK_COMPONENTS
)

install(TARGETS varalyzer-comp
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
