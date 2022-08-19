add_custom_target(LLFileGeneration ALL)

function(generate_ll_file)
  # parse arguments
  set(testfile FILE DEPENDENT)
  cmake_parse_arguments(GEN_LL "${options}" "${testfile}" "" ${ARGN} )
  if(GEN_LL_UNPARSED_ARGUMENTS)
      message(FATAL_ERROR "generate_ll_file has unparsed arguments \"${just_add_UNPARSED_ARGUMENTS}\" did you miss FILE before it?")
  elseif(NOT GEN_LL_FILE)
    message(FATAL_ERROR "generate_ll_file missing FILE to generata")
  elseif(NOT GEN_LL_DEPENDENT)
    message(FATAL_ERROR "generate_ll_file missing DEPENDENT, I dont know which target should depent on the generation of this file.")
  endif()

  # check for mem2reg
  string(FIND "${GEN_LL_FILE}" ".m2r." found_index)
  if ("${found_index}" GREATER -1)
    set(GEN_LL_MEM2REG ON)
  endif()

  # check for debug
  string(FIND "${GEN_LL_FILE}" ".dbg." found_index)
  if ("${found_index}" GREATER -1)
    set(GEN_LL_DEBUG ON)
  endif()

  # check for opt
  string(FIND "${GEN_LL_FILE}" ".o1." found_index)
  if ("${found_index}" GREATER -1)
    set(GEN_LL_O1 ON)
  endif()
  string(FIND "${GEN_LL_FILE}" ".o2." found_index)
  if ("${found_index}" GREATER -1)
    set(GEN_LL_O2 ON)
  endif()
  string(FIND "${GEN_LL_FILE}" ".o3." found_index)
  if ("${found_index}" GREATER -1)
    set(GEN_LL_O3 ON)
  endif()

  # create new path / name
  string(REGEX REPLACE "\.[^.]+$" ".ll" output_path "${GEN_LL_FILE}")
  set(output_path "${CMAKE_CURRENT_BINARY_DIR}/${output_path}")

  # create directory structure if needed
  cmake_path(GET output_path PARENT_PATH parent_path)
  file(MAKE_DIRECTORY "${parent_path}")

  # define compilation flags
  set(GEN_CXX_FLAGS -std=c++17 -fno-discard-value-names -emit-llvm -S)
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
  if(GEN_LL_O1)
    list(APPEND GEN_CXX_FLAGS -O1)
    list(APPEND GEN_C_FLAGS -O1)
    set(GEN_CMD_COMMENT "${GEN_CMD_COMMENT}[O1]")
  elseif(GEN_LL_O2)
    list(APPEND GEN_CXX_FLAGS -O2)
    list(APPEND GEN_C_FLAGS -O2)
    set(GEN_CMD_COMMENT "${GEN_CMD_COMMENT}[O2]")
  elseif(GEN_LL_03)
    list(APPEND GEN_CXX_FLAGS -O3)
    list(APPEND GEN_C_FLAGS -O3)
    set(GEN_CMD_COMMENT "${GEN_CMD_COMMENT}[O3]")
  endif()
  cmake_path(ABSOLUTE_PATH GEN_LL_FILE NORMALIZE OUTPUT_VARIABLE input_path)
  set(GEN_CMD_COMMENT "${GEN_CMD_COMMENT} ${input_path}")

  # What if llvm is shared?
  # Do we need to add lib folders, maybe here LD_LIBRARY_PATH locally (please please dont globally)
  # CONAN_LIB_DIRS <- 
  set(conan_clang)
  set(conan_clangpp)
  set(conan_opt)
  if(CONAN_BIN_DIRS_LLVM)
    foreach(bin_dir ${CONAN_BIN_DIRS_LLVM})
      if (EXISTS "${bin_dir}/clang")
        set(conan_clang "${bin_dir}/clang")
      endif()
      if (EXISTS "${bin_dir}/clang++")
        set(conan_clangpp "${bin_dir}/clang++")
      endif()
      if (EXISTS "${bin_dir}/opt")
        set(conan_opt "${bin_dir}/opt")
      endif()
    endforeach()
  else()
    message(WARNING "Seems like llvm for conan isn't available, fallback to system clang/clang++/opt")
    set(conan_clang clang)
    set(conan_clangpp clang++)
    set(conan_opt opt)
  endif()
  if(NOT conan_clang)
    message(FATAL_ERROR "For .ll generation I would like to use conan clang but I couldn't find it here: ${CONAN_BIN_DIRS_LLVM}")
  elseif(NOT conan_clangpp)
    message(FATAL_ERROR "For .ll generation I would like to use conan clang++ but I couldn't find it here: ${CONAN_BIN_DIRS_LLVM}")
  elseif(NOT conan_opt)
    message(FATAL_ERROR "For .ll generation I would like to use conan opt but I couldn't find it here: ${CONAN_BIN_DIRS_LLVM}")
  endif()

  # define .ll file generation command
  if("${GEN_LL_FILE}" MATCHES "\.cpp$")
    set(GEN_CMD ${conan_clangpp})
    list(APPEND GEN_CMD ${GEN_CXX_FLAGS})
  else()
    set(GEN_CMD ${conan_clang})
    list(APPEND GEN_CMD ${GEN_C_FLAGS})
  endif()
  
  # provide include paths
  set(all_include_dirs ${CONAN_INCLUDE_DIRS})
  list(TRANSFORM all_include_dirs PREPEND "-I")
  
  if(GEN_LL_MEM2REG)
    add_custom_command(
      OUTPUT ${output_path}
      COMMAND ${GEN_CMD} ${all_include_dirs} "${input_path}" -o "${output_path}"
      COMMAND ${CMAKE_CXX_COMPILER_LAUNCHER} ${conan_opt} -mem2reg -S "${output_path}" -o "${output_path}"
      COMMENT ${GEN_CMD_COMMENT}
      DEPENDS ${GEN_LL_FILE}
      VERBATIM
    )
  else()
    add_custom_command(
      OUTPUT ${output_path}
      COMMAND ${GEN_CMD} ${all_include_dirs} "${input_path}" -o "${output_path}"
      COMMENT ${GEN_CMD_COMMENT}
      DEPENDS ${GEN_LL_FILE}
      VERBATIM
    )
  endif()

  string(REGEX REPLACE "[/\\]" "__" test_code_file_target "${GEN_LL_FILE}")
  string(REGEX REPLACE "[^a-zA-Z0-9_-]" "_" test_code_file_target "${test_code_file_target}")
  set(test_code_file_target "${GEN_LL_DEPENDENT}__${test_code_file_target}")
  add_custom_target("${test_code_file_target}"
    DEPENDS ${output_path}
  )
  
  add_dependencies("${GEN_LL_DEPENDENT}" ${test_code_file_target})
endfunction()

file(GLOB_RECURSE test_code_files
     LIST_DIRECTORIES false
     RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
     "*.c*")
list(FILTER test_code_files INCLUDE REGEX "^.*\.(c|cpp)$")

# execute_process(COMMAND clang --version OUTPUT_VARIABLE)
# contains 14. -> allow system llvm bin instead conan llvm bin
foreach(test_code_file ${test_code_files})
    generate_ll_file(
      FILE "${test_code_file}"
      DEPENDENT LLFileGeneration
    )
endforeach()
