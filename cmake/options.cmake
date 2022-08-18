if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build mode ('DebugSan' or 'Debug' or 'Release', default is 'Debug')" FORCE)
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP -fvisibility-inlines-hidden -fstack-protector-strong -ffunction-sections -fdata-sections -pipe")
if(CMAKE_BUILD_TYPE STREQUAL "DebugSan")
    message(STATUS "Selected Debug Build with sanitizers")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fno-omit-frame-pointer -fsanitize=address,undefined")
    set(CMAKE_BUILD_TYPE "Debug")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Selected Debug Build")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
else()
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "Selected Release Build")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()



# Check if we build within the llvm source tree
set(PHASAR_IN_TREE OFF)
if (DEFINED LLVM_MAIN_SRC_DIR)
  set(PHASAR_IN_TREE ON)

  # remove conan llvm dependency, targets are already available
  file(READ "${PROJECT_SOURCE_DIR}/conanfile.txt" conanfile_txt)
  string(REPLACE "\nllvm" "\n#llvm" conanfile_txt "${conanfile_txt}")
  file(WRITE "${PROJECT_SOURCE_DIR}/conanfile.txt" "${conanfile_txt}")

  # fix llvm_test_code expecting clang/clang++/opt in conan directory
  file(READ "${PROJECT_SOURCE_DIR}/phasar/llvm/test/llvm_test_code/CMakeLists.txt" llvm_test_code)
  string(REPLACE "set(conan_clang)" "set(conan_clang clang)" llvm_test_code "${llvm_test_code}")
  string(REPLACE "set(conan_clangpp)" "set(conan_clangpp clang++)" llvm_test_code "${llvm_test_code}")
  string(REPLACE "set(conan_opt)" "set(conan_opt opt)" llvm_test_code "${llvm_test_code}")
  file(WRITE "${PROJECT_SOURCE_DIR}/phasar/llvm/test/llvm_test_code/CMakeLists.txt" "${llvm_test_code}")

  set_property(GLOBAL APPEND PROPERTY LLVM_EXPORTS nlohmann_json_schema_validator)

  # Phasar needs clang headers, specificaly some that are generated by clangs table-gen
  # TODO: migrate this to cmake property reading! Why not simply link against the correct target?
  include_directories(
    ${CLANG_INCLUDE_DIR}
    ${PHASAR_SRC_DIR}/../clang/include
    ${PROJECT_BINARY_DIR}/tools/clang/include
  )

  # TODO: Get all lib targets
  # install(TARGETS ${name}
  #     EXPORT LLVMExports
  #     LIBRARY DESTINATION lib
  #     ARCHIVE DESTINATION lib${LLVM_LIBDIR_SUFFIX})
endif()


# merged PHASAR_BUILD_OPENSSL_TS_UNITTESTS / PHASAR_BUILD_IR
option(PHASAR_BUILD_UNITTESTS "Build all tests and IR files (default is ON)" ON)
set(OPTION_TESTS_DISABLED)
set(OPTION_DOXYGEN_DISABLED)
if (NOT PHASAR_BUILD_UNITTESTS)
  set(OPTION_TESTS_DISABLED "SKIP_SUBDIRECTORIES")
  set(OPTION_DOXYGEN_DISABLED "SKIP_DOXYGEN")
endif()



option(PHASAR_ENABLE_CLANG_TIDY_DURING_BUILD "Run clang-tidy during build (default is OFF)" OFF) # should be fine
if (PHASAR_ENABLE_CLANG_TIDY_DURING_BUILD)
  message(STATUS "Enabled clang-tidy during build")
  set(CMAKE_CXX_CLANG_TIDY
    clang-tidy;
    -header-filter=phasar/.*h$;
    # -warnings-as-errors=*;
  )
endif ()



set(DOXYGEN_PROJECT_BRIEF "Phasar a LLVM-based Static Analysis Framework")
set(DOXYGEN_PROJECT_LOGO "${PROJECT_SOURCE_DIR}/img/Logo_RGB/Phasar_Logo.png")
set(DOXYGEN_ABBREVIATE_BRIEF "")
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${PROJECT_SOURCE_DIR}/README.md")
# set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES) # according to doc if ONLY C files are used
set(DOXYGEN_EXTENSION_MAPPING "cu=C++ cuh=C++")
set(DOXYGEN_HTML_TIMESTAMP "YES")
# set(DOXYGEN_USE_MATHJAX "YES") # needed by option below
# set(DOXYGEN_MATHJAX_RELPATH "https://cdn.jsdelivr.net/npm/mathjax@3") # part of original phasar doxygen but not activated
set(DOXYGEN_HAVE_DOT "YES") # TODO graphviz needs to be there !?
set(DOXYGEN_DOT_MULTI_TARGETS "YES")
set(DOXYGEN_EXCLUDE_PATTERNS "*/llvm_test_code/*")



# TODO is this really needed?
# Basically only works if Phasars public API doesn't expose anything at all which is using the dependency, so a user would need to add the dependency on their own
# if private you have to manually handle include / compile
option(PHASAR_DEBUG_LIBDEPS "Debug internal library dependencies (private linkage)" OFF)



# default option always preset
option(BUILD_SHARED_LIBS "Build shared libraries (default is OFF)" OFF)
option(BUILD_SHARED_LIBS_FORCE "Build shared libraries (default is OFF)" OFF)
if (BUILD_SHARED_LIBS AND NOT BUILD_SHARED_LIBS_FORCE)
  message(FATAL_ERROR "Shared libs needing some modification:\n1. see conanfile.txt\n2. remove all LLVM.* dependencies to only LLVM\n3. use BUILD_SHARED_LIBS_FORCE=ON\n")
endif()



option(PHASAR_ENABLE_WARNINGS "Enable warnings" ON)
if (PHASAR_ENABLE_WARNINGS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-return-type-c-linkage ")
endif()




option(PHASAR_ENABLE_PIC "Build Position-Independed Code" ON)
if (PHASAR_ENABLE_PIC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()



if (NOT PHASAR_ENABLE_PAMM)
  set(PHASAR_ENABLE_PAMM "Off" CACHE STRING "Enable the performance measurement mechanism ('Off', 'Core' or 'Full', default is 'Off')" FORCE)
  set_property(CACHE PHASAR_ENABLE_PAMM PROPERTY STRINGS "Off" "Core" "Full")
endif()
if(PHASAR_BUILD_UNITTESTS)
  message("PAMM metric severity level: Off (due to unittests)")
elseif(PHASAR_ENABLE_PAMM STREQUAL "Core")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPAMM_CORE")
  message("PAMM metric severity level: Core")
elseif(PHASAR_ENABLE_PAMM STREQUAL "Full")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPAMM_FULL")
  message("PAMM metric severity level: Full")
else()
  message("PAMM metric severity level: Off")
endif()



option(PHASAR_ENABLE_DYNAMIC_LOG "Makes it possible to switch the logger on and off at runtime (default is ON)" ON)
if (PHASAR_ENABLE_DYNAMIC_LOG)
  message(STATUS "Dynamic log enabled")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDYNAMIC_LOG")
else()
  message(STATUS "Dynamic log disabled")
endif()



if (LLVM_ENABLE_LIBCXX)
  set(PHASAR_STD_FILESYSTEM c++fs)
else()
  set(PHASAR_STD_FILESYSTEM stdc++fs)
endif()
