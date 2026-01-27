
function(phasar_link_llvm executable)
  # llvm_config links LLVM as PRIVATE. We need to link PUBLIC
  if (USE_LLVM_FAT_LIB)
    target_link_libraries(${executable} PUBLIC LLVM)
  else()
    llvm_map_components_to_libnames(LLVM_LIBRARIES ${ARGN})
    target_link_libraries(${executable} PUBLIC ${LLVM_LIBRARIES})
  endif()
endfunction()

macro(add_cxx_compile_definitions)
  add_compile_definitions("$<$<COMPILE_LANGUAGE:CXX>:${ARGN}>")
endmacro()

function(add_phasar_unittest test_name)
  message("Set-up unittest: ${test_name}")
  get_filename_component(test ${test_name} NAME_WE)
  add_executable(${test}
    ${test_name}
  )
  add_dependencies(PhasarUnitTests ${test})

  phasar_link_llvm(${test} ${LLVM_LINK_COMPONENTS})

  target_link_libraries(${test}
    PRIVATE
      phasar
      GTest::gtest
  )

  add_test(NAME "${test}"
    COMMAND ${test} ${CATCH_TEST_FILTER}
    WORKING_DIRECTORY ${PHASAR_UNITTEST_DIR}
  )
  set_tests_properties("${test}" PROPERTIES LABELS "all")
  set(CTEST_OUTPUT_ON_FAILURE ON)

  if (CODE_COVERAGE)
    target_code_coverage(${test} AUTO ALL)
  endif()
endfunction()

function(validate_binary_version result item)
  message(STATUS "  - validate_binary_version of \"${item}\"")
  execute_process(
    COMMAND "${item}" --version
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  string(REGEX MATCH " version ${PHASAR_LLVM_VERSION}\\." match "${output}")
  if (match)
    set(${result} TRUE PARENT_SCOPE)
  else()
    set(${result} FALSE PARENT_SCOPE)
  endif()
endfunction()

function(generate_ll_file)
  set(options MEM2REG DEBUG O1 O2 O3)
  set(testfile FILE)
  cmake_parse_arguments(GEN_LL "${options}" "${testfile}" "" ${ARGN})

  if (NOT clang)
    # Conan deps are available in in PATH
    set(default_llvm "${LLVM_TOOLS_BINARY_DIR}" )
    set(fallback_llvm "${Clang_INCLUDE_DIR}/../bin" "${LLVM_INCLUDE_DIR}/../bin")
    set(user_compiled_llvm "/usr/local/llvm-${PHASAR_LLVM_VERSION}/bin")
    set(package_manager_llvm "/usr/lib/llvm-${PHASAR_LLVM_VERSION}/bin/")
    foreach(hint ${default_llvm} ${fallback_llvm} ${user_compiled_llvm} ${package_manager_llvm})
      if ("${CMAKE_VERSION}" VERSION_GREATER_EQUAL "3.20")
        cmake_path(NORMAL_PATH hint OUTPUT_VARIABLE hint)
      endif()
      list(APPEND binary_hint_paths "${hint}")
    endforeach()
    message(STATUS "HINTS to find clang/clang++/opt: ${binary_hint_paths}")

    if ("${CMAKE_VERSION}" VERSION_LESS "3.25") # VALIDATOR requires it
      message(WARNING "I would prefer CMake >= 3.25 but I will try my best to resolve deps.")
      find_program(clang REQUIRED
        NAMES clang-${PHASAR_LLVM_VERSION} clang
        HINTS ${binary_hint_paths})
      find_program(clangcpp REQUIRED
        NAMES clang++-${PHASAR_LLVM_VERSION} clang++
        HINTS ${binary_hint_paths})
      find_program(opt REQUIRED
        NAMES opt-${PHASAR_LLVM_VERSION} opt
        HINTS ${binary_hint_paths})

      set(IS_VALID_VERSION "")
      validate_binary_version(IS_VALID_VERSION "${clang}")
      if (NOT "${IS_VALID_VERSION}")
        set(clang "")
      endif()
      validate_binary_version(IS_VALID_VERSION "${clangcpp}")
      if (NOT "${IS_VALID_VERSION}")
        set(clangcpp "")
      endif()
      validate_binary_version(IS_VALID_VERSION "${opt}")
      if (NOT "${IS_VALID_VERSION}")
        set(opt "")
      endif()
    else()
      find_program(clang REQUIRED
        NAMES clang-${PHASAR_LLVM_VERSION} clang
        HINTS ${binary_hint_paths}
        VALIDATOR validate_binary_version)
      find_program(clangcpp REQUIRED
        NAMES clang++-${PHASAR_LLVM_VERSION} clang++
        HINTS ${binary_hint_paths}
        VALIDATOR validate_binary_version)
      find_program(opt REQUIRED
        NAMES opt-${PHASAR_LLVM_VERSION} opt
        HINTS ${binary_hint_paths}
        VALIDATOR validate_binary_version)
    endif()
    if ("${clang}" STREQUAL "")
      message(FATAL_ERROR "Couldn't find clang in version ${PHASAR_LLVM_VERSION}")
    else()
      message(STATUS "found clang binary in \"${clang}\"")
    endif()
    if ("${clangcpp}" STREQUAL "")
      message(FATAL_ERROR "Couldn't find clang++ in version ${PHASAR_LLVM_VERSION}")
    else()
      message(STATUS "found clang++ binary in \"${clangcpp}\"")
    endif()
    if ("${opt}" STREQUAL "")
      message(FATAL_ERROR "Couldn't find opt in version ${PHASAR_LLVM_VERSION}")
    else()
      message(STATUS "found opt binary in \"${opt}\"")
    endif()
  endif()

  # get file extension
  get_filename_component(test_code_file_ext ${GEN_LL_FILE} EXT)
  string(REPLACE "." "_" ll_file_suffix ${test_code_file_ext})

  # define .ll file name
  # set(ll_file_suffix "_${test_code_file_ext}")
  if(GEN_LL_MEM2REG)
    set(ll_file_suffix "${ll_file_suffix}_m2r")
  endif()

  if(GEN_LL_DEBUG)
    set(ll_file_suffix "${ll_file_suffix}_dbg")
  endif()

  # set(ll_file_suffix "${ll_file_suffix}.ll")
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
  set(GEN_CXX_FLAGS -std=c++17 -fno-discard-value-names -emit-llvm -S -w)
  set(GEN_C_FLAGS -fno-discard-value-names -emit-llvm -S -w)
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

  if(GEN_LL_O1)
    list(APPEND GEN_CXX_FLAGS -O1)
    list(APPEND GEN_C_FLAGS -O1)
    set(GEN_CMD_COMMENT "${GEN_CMD_COMMENT}[O1]")
  endif()

  if(GEN_LL_O2)
    list(APPEND GEN_CXX_FLAGS -O2)
    list(APPEND GEN_C_FLAGS -O2)
    set(GEN_CMD_COMMENT "${GEN_CMD_COMMENT}[O2]")
  endif()

  if(GEN_LL_03)
    list(APPEND GEN_CXX_FLAGS -O3)
    list(APPEND GEN_C_FLAGS -O3)
    set(GEN_CMD_COMMENT "${GEN_CMD_COMMENT}[O3]")
  endif()

  set(GEN_CMD_COMMENT "${GEN_CMD_COMMENT} ${GEN_LL_FILE}")

  # define .ll file generation command
  if(${test_code_file_ext} STREQUAL ".cpp")
    set(GEN_CMD ${CMAKE_CXX_COMPILER_LAUNCHER} ${clangcpp})
    list(APPEND GEN_CMD ${GEN_CXX_FLAGS})
  else()
    set(GEN_CMD ${CMAKE_C_COMPILER_LAUNCHER} ${clang})
    list(APPEND GEN_CMD ${GEN_C_FLAGS})
  endif()

  if(GEN_LL_MEM2REG)
      add_custom_command(
      OUTPUT ${test_code_ll_file}
      COMMAND ${GEN_CMD} ${test_code_file_path} -o ${test_code_ll_file}
      COMMAND ${CMAKE_CXX_COMPILER_LAUNCHER} ${opt} -p mem2reg -S ${test_code_ll_file} -o ${test_code_ll_file}
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
  add_llvm_executable(${name} ${ARGN})
  set_target_properties(${name} PROPERTIES FOLDER "phasar_executables")
  install(TARGETS ${name}
    RUNTIME DESTINATION bin
  )
endmacro(add_phasar_executable)

function(add_phasar_library name)
  set(PHASAR_LIB_OPTIONS SHARED STATIC MODULE INTERFACE)
  set(PHASAR_LIB_MULTIVAL LLVM_LINK_COMPONENTS LINKS LINK_PUBLIC LINK_PRIVATE FILES MODULE_FILES)
  cmake_parse_arguments(PHASAR_LIB "${PHASAR_LIB_OPTIONS}" "" "${PHASAR_LIB_MULTIVAL}" ${ARGN})
  set(srcs ${PHASAR_LIB_UNPARSED_ARGUMENTS})
  list(APPEND srcs ${PHASAR_LIB_FILES})

  if(MSVC_IDE OR XCODE)
    file(GLOB_RECURSE headers *.h *.td *.def)
    list(APPEND srcs ${headers})
    string(REGEX MATCHALL "/[^/]" split_path ${CMAKE_CURRENT_SOURCE_DIR})
    list(GET split_path -1 dir)
    file(GLOB_RECURSE headers
      ../../include/phasar${dir}/*.h)
    list(APPEND srcs ${headers})
  endif(MSVC_IDE OR XCODE)

  if(PHASAR_LIB_MODULE)
    set(libkind MODULE)
  elseif(PHASAR_LIB_INTERFACE)
    set(libkind INTERFACE)
  elseif(PHASAR_LIB_SHARED OR BUILD_SHARED_LIBS)
    set(libkind SHARED)
  else()
    set(libkind STATIC)
  endif()

  # cut off prefix phasar_ for convenient component names
  string(REGEX REPLACE phasar_ "" component_name ${name})

  add_library(${name} ${libkind} ${srcs})
  add_library(phasar::${component_name} ALIAS ${name})
  set_target_properties(${name} PROPERTIES
    EXPORT_NAME ${component_name}
  )

  target_compile_features(${name} PUBLIC cxx_std_20)

  set(install_module)
  if(PHASAR_LIB_MODULE_FILES)
    if(PHASAR_BUILD_MODULES)
      target_sources(${name} PUBLIC
        FILE_SET cxx_modules
        TYPE CXX_MODULES
        FILES ${PHASAR_LIB_MODULE_FILES}
      )

      target_compile_features(${name} PUBLIC cxx_std_20)

      set(install_module FILE_SET cxx_modules DESTINATION ${CMAKE_INSTALL_LIBDIR})
    elseif(NOT srcs)
      # Add dummy src to prevent cmake error
      set(dummy_src "${CMAKE_CURRENT_BINARY_DIR}/${name}_dummysrc.cpp")
      if(NOT EXISTS "${dummy_src}")
        file(WRITE "${dummy_src}" "")
      endif()
      target_sources(${name} PRIVATE "${dummy_src}")
    endif()
  endif()

  if(LLVM_COMMON_DEPENDS)
    add_dependencies(${name} ${LLVM_COMMON_DEPENDS})
  endif(LLVM_COMMON_DEPENDS)

  target_link_libraries(${name} PUBLIC ${PHASAR_LIB_LINKS} phasar_interface ${PHASAR_LIB_LINK_PUBLIC})
  if(PHASAR_DEBUG_LIBDEPS)
    target_link_libraries(${name} PRIVATE -Wl,-z,defs)
  endif()

  target_link_libraries(${name} PRIVATE ${PHASAR_LIB_LINK_PRIVATE})

  phasar_link_llvm(${name} ${PHASAR_LIB_LLVM_LINK_COMPONENTS})

  # Library Dependency Dirs
  if(NOT PHASAR_IN_TREE)
    target_include_directories(${name} PUBLIC
      ${LLVM_INCLUDE_DIRS}
    )
    target_link_directories(${name} PUBLIC
      ${LLVM_LIB_PATH} ${LLVM_LIBRARY_DIRS}
    )
  endif()

  if(MSVC)
    get_target_property(cflag ${name} COMPILE_FLAGS)

    if(NOT cflag)
      set(cflag "")
    endif(NOT cflag)

    set(cflag "${cflag} /Za")
    set_target_properties(${name} PROPERTIES COMPILE_FLAGS ${cflag})
  endif(MSVC)

  if(PHASAR_IN_TREE)
    install(TARGETS ${name}
      EXPORT LLVMExports
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib${LLVM_LIBDIR_SUFFIX}
      ${install_module}
    )
  else()
    install(TARGETS ${name}
      EXPORT PhasarExports
      # NOTE: Library, archive and runtime destination are automatically set by
      # GNUInstallDirs which is included in the top-level CMakeLists.txt

      ${install_module}
    )
  endif()

  set_property(GLOBAL APPEND PROPERTY LLVM_EXPORTS ${name})

  if (CODE_COVERAGE)
    target_code_coverage(${name} AUTO ALL)
  endif()
endfunction(add_phasar_library)

function(subdirlist result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")

  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()

  set(${result} ${dirlist} PARENT_SCOPE)
endfunction(subdirlist)
