function(add_phasar_unittest test_name)
  message("Set-up unittest: ${test_name}")
  get_filename_component(test ${test_name} NAME_WE)
  add_executable(${test}
    ${test_name}
  )
  target_link_libraries(${test}
  phasar_config
  phasar_controller
  phasar_db
  phasar_experimental
  phasar_clang
  phasar_controlflow
  phasar_ifdside
  phasar_mono
  phasar_passes
  phasar_plugins
  phasar_pointer
  phasar_phasarllvm_utils
  phasar_utils
  boost_program_options
  boost_filesystem
  boost_graph
  boost_system
  boost_log
  boost_thread
  ${SQLITE3_LIBRARY}
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
  ${CLANG_LIBRARIES}  
  ${llvm_libs}
  curl
  gtest
)
add_test(NAME "${test}"
  COMMAND ${test} ${CATCH_TEST_FILTER}
)
set_tests_properties("${test}" PROPERTIES LABELS "all")
set(CTEST_OUTPUT_ON_FAILURE TRUE)
endfunction()

macro(add_phasar_executable name)
  set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/bin)
  set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib)
  add_llvm_executable( ${name} ${ARGN} )
  set_target_properties(${name} PROPERTIES FOLDER "phasar_executables")
  install (TARGETS ${name}
    RUNTIME DESTINATION bin
  )
endmacro(add_phasar_executable)

macro(add_phasar_library name)
  set(srcs ${ARGN})
  if(MSVC_IDE OR XCODE)
    file( GLOB_RECURSE headers *.h *.td *.def)
    set(srcs ${srcs} ${headers})
    string( REGEX MATCHALL "/[^/]" split_path ${CMAKE_CURRENT_SOURCE_DIR})
    list( GET split_path -1 dir)
    file( GLOB_RECURSE headers
      ../../include/polly${dir}/*.h)
    set(srcs ${srcs} ${headers})
  endif(MSVC_IDE OR XCODE)
  if (MODULE)
    set(libkind MODULE)
  elseif (SHARED_LIBRARY)
    set(libkind SHARED)
  else()
    set(libkind)
  endif()
  add_library( ${name} ${libkind} ${srcs} )
  if( LLVM_COMMON_DEPENDS )
    add_dependencies( ${name} ${LLVM_COMMON_DEPENDS} )
  endif( LLVM_COMMON_DEPENDS )
  if( LLVM_USED_LIBS )
    foreach(lib ${LLVM_USED_LIBS})
      target_link_libraries( ${name} ${lib} )
    endforeach(lib)
  endif( LLVM_USED_LIBS )

  if(PHASAR_LINK_LIBS)
    foreach(lib ${PHASAR_LINK_LIBS})
      target_link_libraries(${name} LINK_PRIVATE ${lib})
    endforeach(lib)
  endif(PHASAR_LINK_LIBS)

  if( LLVM_LINK_COMPONENTS )
    llvm_config(${name} ${LLVM_LINK_COMPONENTS})
  endif( LLVM_LINK_COMPONENTS )
  if(MSVC)
    get_target_property(cflag ${name} COMPILE_FLAGS)
    if(NOT cflag)
      set(cflag "")
    endif(NOT cflag)
    set(cflag "${cflag} /Za")
    set_target_properties(${name} PROPERTIES COMPILE_FLAGS ${cflag})
  endif(MSVC)
  install(TARGETS ${name}
    EXPORT LLVMExports
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib${LLVM_LIBDIR_SUFFIX})
  set_property(GLOBAL APPEND PROPERTY LLVM_EXPORTS ${name})
endmacro(add_phasar_library)

macro(add_phasar_loadable_module name)
  set(srcs ${ARGN})
  # klduge: pass different values for MODULE with multiple targets in same dir
  # this allows building shared-lib and module in same dir
  # there must be a cleaner way to achieve this....
  if (MODULE)
  else()
    set(GLOBAL_NOT_MODULE TRUE)
  endif()
  set(MODULE TRUE)
  add_phasar_library(${name} ${srcs})
  if (GLOBAL_NOT_MODULE)
    unset (MODULE)
  endif()
  if (APPLE)
    # Darwin-specific linker flags for loadable modules.
    set_target_properties(${name} PROPERTIES
      LINK_FLAGS "-Wl,-flat_namespace -Wl,-undefined -Wl,suppress")
  endif()
endmacro(add_phasar_loadable_module)