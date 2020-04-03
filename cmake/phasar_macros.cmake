function(add_phasar_unittest test_name)
  message("Set-up unittest: ${test_name}")
  get_filename_component(test ${test_name} NAME_WE)
  add_executable(${test}
    ${test_name}
  )

  target_link_libraries(${test}
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
    # ${CLANG_LIBRARIES}
    ${llvm_libs}
    curl
    gtest
  )

  add_test(NAME "${test}"
    COMMAND ${test} ${CATCH_TEST_FILTER}
  )
  set_tests_properties("${test}" PROPERTIES LABELS "all")
  set(CTEST_OUTPUT_ON_FAILURE ON)
endfunction()

function(generate_ll_file)
  set(options MEM2REG DEBUG)
  set(testfile FILE)
  cmake_parse_arguments(GEN_LL "${options}" "${testfile}" "" ${ARGN} )
  # get file extension
  get_filename_component(test_code_file_ext ${GEN_LL_FILE} EXT)
  string(REPLACE "." "_" ll_file_suffix ${test_code_file_ext})
  # define .ll file name
#  set(ll_file_suffix "_${test_code_file_ext}")
  if(GEN_LL_MEM2REG)
    set(ll_file_suffix "${ll_file_suffix}_m2r")
  endif()
  if(GEN_LL_DEBUG)
    set(ll_file_suffix "${ll_file_suffix}_dbg")
  endif()
#  set(ll_file_suffix "${ll_file_suffix}.ll")
  string(REPLACE ${test_code_file_ext}
    "${ll_file_suffix}.ll" test_code_ll_file
    ${GEN_LL_FILE}
  )
  # get file path
  set(test_code_file_path "${CMAKE_CURRENT_SOURCE_DIR}/${GEN_LL_FILE}")

  # define custom target name
  # target name = parentdir + test code file name + mem2reg + debug
  get_filename_component(parent_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)
  get_filename_component(test_code_file_name ${GEN_LL_FILE} NAME_WE)
  set(test_code_file_target "${parent_dir}_${test_code_file_name}${ll_file_suffix}")

  # define compilation flags
  set(GEN_CXX_FLAGS -std=c++14 -fno-discard-value-names -emit-llvm -S)
  set(GEN_C_FLAGS -fno-discard-value-names -emit-llvm -S)
  set(GEN_CMD_COMMENT "[LL]")
  if(GEN_LL_MEM2REG)
    list(APPEND GEN_CXX_FLAGS -Xclang -disable-O0-optnone)
    list(APPEND GEN_C_FLAGS -Xclang -disable-O0-optnone)
    set(GEN_CMD_COMMENT "${GEN_CMD_COMMENT}[M2R]")
  endif()
  if(GEN_LL_DEBUG)
    list(APPEND GEN_CXX_FLAGS -g)
    list(APPEND GEN_C_FLAGS -g)
    set(GEN_CMD_COMMENT "${GEN_CMD_COMMENT}[DBG]")
  endif()
  set(GEN_CMD_COMMENT "${GEN_CMD_COMMENT} ${GEN_LL_FILE}")

  # define .ll file generation command
  if(${test_code_file_ext} STREQUAL ".cpp")
    set(GEN_CMD ${CMAKE_CXX_COMPILER_LAUNCHER} ${CMAKE_CXX_COMPILER})
    list(APPEND GEN_CMD ${GEN_CXX_FLAGS})
  else()
    set(GEN_CMD ${CMAKE_C_COMPILER_LAUNCHER} ${CMAKE_C_COMPILER})
    list(APPEND GEN_CMD ${GEN_C_FLAGS})
  endif()
  if(GEN_LL_MEM2REG)
    add_custom_command(
      OUTPUT ${test_code_ll_file}
      COMMAND ${GEN_CMD} ${test_code_file_path} -o ${test_code_ll_file}
      COMMAND ${CMAKE_CXX_COMPILER_LAUNCHER} opt -mem2reg -S ${test_code_ll_file} -o ${test_code_ll_file}
      COMMENT ${GEN_CMD_COMMENT}
      DEPENDS ${GEN_LL_FILE}
      VERBATIM
    )
  else()
    add_custom_command(
    OUTPUT ${test_code_ll_file}
    COMMAND ${GEN_CMD} ${test_code_file_path} -o ${test_code_ll_file}
    COMMENT ${GEN_CMD_COMMENT}
    DEPENDS ${GEN_LL_FILE}
    VERBATIM
    )
  endif()
  add_custom_target(${test_code_file_target}
    DEPENDS ${test_code_ll_file}
  )
  add_dependencies(LLFileGeneration ${test_code_file_target})
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
      ../../include/phasar${dir}/*.h)
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
  add_library( phasar::${name} ALIAS ${name} )
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
      if(PHASAR_DEBUG_LIBDEPS)
        target_link_libraries(${name} LINK_PRIVATE ${lib})
      else()
        target_link_libraries(${name} LINK_PUBLIC ${lib})
      endif(PHASAR_DEBUG_LIBDEPS)
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
  install(TARGETS ${name}
    EXPORT phasarTargets
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

macro(subdirlist result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist})
endmacro(subdirlist)
