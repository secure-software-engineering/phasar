# just simple cmake
#
# main commands: just_add_library() just_add_executable() just_add_tests()
# - target name generated "project-folder[-test]"
# - sources are taken by globbing
#   - INCLUDE "^.*/namespace/.*$" ... is applied before EXCLUDE
#   - EXCLUDE "^.*TEST$" ... is applied after INCLUDE
# - LINK [lib1 ...] (tests linking automatically against target under test)
# - DEPENDS [custom_command_1 ...]
# - SKIP_SUBDIRECTORIES if a subfolder contains a CMakeLists.txt its automatically added, but not with this argument
# - SUFFIX "asd" project-folder -> project-folder-asd 
# 
# util commands:
# - just_limit_jobs(COMPILE 1000 LINK 4000) calculates the compile / link job pools according to your current available memory.
#   You configure the amount of expected memory usage in mb per compile / link job. 
#   Get rid of out-of-memory crashes, manually setting jobs with `ninja -j x`.
# - just_copy_resource(project-NAME) copies resources from given target into current binary dir
# 
# other features:
# + resource/config/server.json -> just create the structure and your test will find it in config/server.json
# + run tests with ctest (Labels = target under test)
# + generate Coverage with cmake ... -DCOVERAGE=ON ninja ccov-all
# + conanfile.txt handling + auto generated if not present
# + doxygen incremental generation
#
# setup:
# 0. pip install conan
# 1. Copy to  your project `cmake/just-simple.cmake`
# 2. Your project invokes before include: [`project(NAME)`](https://cmake.org/cmake/help/latest/command/project.html)
# 3. Include it from your main file `include("cmake/just-simple.cmake")`
# 4. Target names are generated `project-folder[-test]` therefore suggested folder structure:
# .
# ├── CMakeLists.txt // project(...), include("cmake/just-simple.cmake"), add_subdirector(project/target1), ...
# ├── CMakePresets
# ├── conanfile.txt // [requires] sqlite3/[>=3.30.0 <4.0.0]
# ├── cmake
# │   └── just-simple.cmake
# └── project
#     ├── target1 // project-target1
#     │   ├── CMakeLists.txt // just_add_executable(LINK project-target2)
#     │   ├── include
#     │   │   └── namespace
#     │   │       └── public_include.hpp
#     │   └── src
#     │       └── namespace
#     │           └── main.cpp
#     └── target2 // project-target2
#         ├── CMakeLists.txt // just_add_library(LINK sqlite3) test folder autoloaded because it contains CMakeLists.txt
#         ├── include
#         │   └── namespace
#         │       └── lib.h
#         ├── resource
#         │   └── config
#         │       └── server.cfg
#         ├── src
#         │   └── namespace
#         │       └── lib.cpp
#         └── test // project-target2-test
#             ├── CMakeLists.txt // just_add_tests() auto linked vs project-target2 gtest_main, gmock_main
#             ├── include
#             │   └── .gitkeep
#             ├── resource
#             │   └── config
#             │       └── test.cfg // tests can load(config/test.cfg) and load(config/server.cfg)
#             └── src
#                 └── simple_test.cpp

if (${CMAKE_MINIMUM_REQUIRED_VERSION} VERSION_LESS "3.21") # for --output-junit
    message(FATAL_ERROR "
        just-simple.cmake requires min cmake version 3.21
        if this is an issue you can quick fix it with conan:
        conan install \"cmake/[>3.21 <4.0]@\" -g virtualenv
        source activate_run.sh
    ")
endif()

include(GNUInstallDirs)
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif ()

# download cmake dependencies if not present
set(JUST_SIMPLE_conan "${CMAKE_CURRENT_LIST_DIR}/conan.cmake")
if(NOT EXISTS "${JUST_SIMPLE_conan}")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
        "${JUST_SIMPLE_conan}"
        EXPECTED_HASH SHA256=5cdb3042632da3efff558924eecefd580a0e786863a857ca097c3d1d43df5dcd
        TLS_VERIFY ON)
endif()
include(${JUST_SIMPLE_conan})

set(JUST_SIMPLE_coverage "${CMAKE_CURRENT_LIST_DIR}/code-coverage.cmake")
if (NOT EXISTS "${JUST_SIMPLE_coverage}")
    file(DOWNLOAD "https://raw.githubusercontent.com/StableCoder/cmake-scripts/ed79fb95f7bd39b2cb158d7ff83a5dbb639343e8/code-coverage.cmake"
        "${JUST_SIMPLE_coverage}"
        EXPECTED_HASH SHA256=58900f7280f43cab6c66e793bc8cd8aa06c9c8b4d6f0c10dd5e86e19efb46718
        TLS_VERIFY ON)
endif()

# setup conan
# create basic conanfile.txt if not present
if (NOT CONAN_EXPORTED) # only if conan not already set up
    if (NOT CONANFILE)
        set(CONANFILE "${PROJECT_SOURCE_DIR}/conanfile.txt")
    endif()
    if (NOT EXISTS "${CONANFILE}")
        file(WRITE "${CONANFILE}" "# doc: https://docs.conan.io/en/latest/reference/conanfile_txt.html\n\n")
        file(APPEND "${CONANFILE}" "[requires]\ngtest/1.12.1\ndoxygen/1.9.4\n#package_name/version@user/channel (default @_/_)\n\n")
        file(APPEND "${CONANFILE}" "[generators]\ncmake\n\n")
        file(APPEND "${CONANFILE}" "[options]\n#package_name:shared=False\n\n")
    endif()
    conan_cmake_run(
        BASIC_SETUP
        CONANFILE "${CONANFILE}"
        BUILD missing)
else()
    conan_cmake_run(BASIC_SETUP)
endif()

# debugging CMAKE https://cliutils.gitlab.io/modern-cmake/chapters/features/debug.html
#include(CMakePrintHelpers)
# cmake_print_variables(MY_VARIABLE)
# cmake_print_properties(TARGETS my_target PROPERTIES POSITION_INDEPENDENT_CODE)
# --warn-uninitialized --trace --trace-expand

# before "limit linking jobs", sets the same property rather than extending
# but its a dependency so we shouldnt modify it, will break on update
include("${JUST_SIMPLE_coverage}")
add_code_coverage_all_targets(EXCLUDE ".conan/.*" ".*/test/.*") # TODO its allowed to call it something else, move name enforcing to config?

enable_testing() #enable ctest
set(CMAKE_CTEST_ARGUMENTS "--output-junit;${CMAKE_BINARY_DIR}/Testing/Temporary/JUnit.xml;--output-on-failure;") # for ci import
include(GoogleTest)

# generates compile_commands.json used by IDEs to resolve includes aso
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# TODO offer a diagnostic run with link / compile = 1, job = 1 and check mem usage with "sar -r cmd" (sysstat package) automatically?
function(just_limit_jobs)
    # Argument parsing
    set(options )
    set(oneValueArgs LINK COMPILE)
    set(multiValueArgs )
    cmake_parse_arguments(PARSE_ARGV "0" "just_limit" "${options}" "${oneValueArgs}" "${multiValueArgs}")

    if(just_limit_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "target \"${TARGET}\" has unparsed arguments \"${just_limit_UNPARSED_ARGUMENTS}\", LINK \"expected ram usage per link process\" COMPILE \"expected ram usage per compile process\"?")
    endif()

    cmake_host_system_information(RESULT system_ram QUERY AVAILABLE_PHYSICAL_MEMORY)
    # linking / compiling is done at the same time and to much memory consumption will break ci / slow done it a lot
    # upper limit for link/compile job is automatically set by job count
    math(EXPR system_ram_possible_jobs "${system_ram} / ${just_limit_LINK}" OUTPUT_FORMAT DECIMAL)
    if (system_ram_possible_jobs LESS 1)
        set(system_ram_possible_jobs 1)
    endif()
    cmake_host_system_information(RESULT system_cores QUERY NUMBER_OF_LOGICAL_CORES)

    message(STATUS "resources: cores=${system_cores} memory=${system_ram}")
    if (system_ram_possible_jobs LESS system_cores)
        math(EXPR compile_jobs "${system_ram} / ${just_limit_COMPILE}" OUTPUT_FORMAT DECIMAL)
        if (compile_jobs LESS 1)
            set(compile_jobs 1)
        endif()
        message("Your System is Memory limited, linking jobs: ${system_ram_possible_jobs} compile jobs: ${compile_jobs}")
        set_property(GLOBAL APPEND PROPERTY JOB_POOLS linking=${system_ram_possible_jobs} compiling=${compile_jobs})
    else()
        message("Your System is CPU limited, linking jobs: ${system_cores} compile jobs: ${system_cores}")
        set_property(GLOBAL APPEND PROPERTY JOB_POOLS linking=${system_cores} compiling=${system_cores})
    endif()

    if (NOT CMAKE_JOB_POOL_LINK)
        set(CMAKE_JOB_POOL_LINK linking PARENT_SCOPE)
    else()
        message("Using provided job pool list for linking: ${CMAKE_JOB_POOL_LINK}")
    endif()
    if (NOT CMAKE_JOB_POOL_COMPILE)
        set(CMAKE_JOB_POOL_COMPILE compiling PARENT_SCOPE)
    else()
        message("Using provided job pool list for linking: ${CMAKE_JOB_POOL_COMPILE}")
    endif()
endfunction()

function(_just_add_check TARGET)
    if (TARGET "${TARGET}")
        message(FATAL_ERROR "cant add executable because the target ${TARGET} is already defined, please create just one executable per directory")
    endif()

    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
    else()
        message(FATAL_ERROR "_just_add_check: ${TARGET} has no include folder, please add it")
    endif()

    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src")
    else()
        message(FATAL_ERROR "_just_add_check: ${TARGET} has no src folder, please add it")
    endif()
endfunction()

function(_just_add_subdirectory)
  FILE(GLOB files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

  foreach(file ${files})
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${file}/CMakeLists.txt")
        add_subdirectory("${file}")
    endif()
    endforeach()
endfunction()

function(_just_add_resource SRC)
    if (EXISTS "${SRC}/resource")
        message(NOTICE "found resources in: ${SRC}/resource")
        file(GLOB_RECURSE files RELATIVE "${SRC}/resource" LIST_DIRECTORIES false "${SRC}/resource/*")
        foreach(file ${files})
            configure_file("${SRC}/resource/${file}" "${CMAKE_CURRENT_BINARY_DIR}/${file}" COPYONLY)
        endforeach()
    else()
        message(NOTICE "no resource folder: ${SRC}/resource")
    endif()
endfunction()

function(just_copy_resource TARGET)
    get_target_property(target_folder "${TARGET}" SOURCE_DIR)
    _just_add_resource("${target_folder}")
endfunction()

function(_just_check_library_order)
    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs LINK)
    cmake_parse_arguments(PARSE_ARGV "0" "just_check" "${options}" "${oneValueArgs}" "${multiValueArgs}")

    set(index -1)
    set(wrong_order OFF)
    set(correct_order)
    foreach(available_lib ${CONAN_LIBS})
        list(FIND just_check_LINK ${available_lib} found)
        if (found GREATER -1)
            list(APPEND correct_order "${available_lib}")
            if (found LESS index)
                set(wrong_order ON)
            endif()
            set(index "${found}")
        endif()
    endforeach()

    if (${wrong_order})
        message(WARNING "${just_check_TARGET} LINK order looks incorrect, expecting: ${correct_order}")
    endif()
endfunction()

function(just_add_library)
    # Argument parsing
    set(options SKIP_SUBDIRECTORIES SKIP_DOXYGEN)
    set(oneValueArgs SUFFIX)
    set(multiValueArgs INCLUDE EXCLUDE LINK DEPENDS)
    cmake_parse_arguments(PARSE_ARGV "0" "just_add" "${options}" "${oneValueArgs}" "${multiValueArgs}")

    # get name from
    get_filename_component(TARGET "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
    set(TARGET "${CMAKE_PROJECT_NAME}-${TARGET}${just_add_SUFFIX}")
    if(just_add_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "target \"${TARGET}\" has unparsed arguments \"${just_add_UNPARSED_ARGUMENTS}\" maybe LINK in front of it?")
    endif()
    _just_add_check("${TARGET}")

    if (just_add_LINK)
        _just_check_library_order(TARGET "${TARGET}" LINK ${just_add_LINK})
    endif()

    # create filelist
    file(GLOB_RECURSE files include/* src/*)
    list(FILTER files EXCLUDE REGEX "^.*/\.gitkeep$") #XXX filter others to? only whitelist for .cpp? Variable for it?
    if(just_add_INCLUDE)
        foreach(pattern ${just_add_INCLUDE})
            set(just_add_temp ${files})
            list(FILTER just_add_temp INCLUDE REGEX ${pattern})
            list(APPEND all_includes ${just_add_temp})
        endforeach()
        set(files ${all_includes})
    endif()
    if(just_add_EXCLUDE)
        foreach(pattern ${just_add_EXCLUDE})
            list(FILTER files EXCLUDE REGEX ${pattern})
        endforeach()
    endif()
    
    set(is_header_only OFF)
    list(APPEND files_src ${files})
    list(FILTER files_src EXCLUDE REGEX "^.*/include/.*$")
    if (NOT files_src)
        set(is_header_only ON)
    endif()

    # create target
    if(is_header_only)
        message(NOTICE "just_add_library ${TARGET}: header only library, skip linking")
        add_library("${TARGET}" INTERFACE ${files})
        target_include_directories("${TARGET}" INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include" )
        # XXX for header only libs, but really needed?
        set_target_properties("${TARGET}" PROPERTIES LINKER_LANGUAGE CXX)
        if (just_add_LINK)
            message(NOTICE "just_add_library ${TARGET}: linking to ${just_add_LINK}")
            target_link_libraries("${TARGET}" INTERFACE ${just_add_LINK})
        endif()
    else()
        add_library("${TARGET}" ${files})
        target_include_directories("${TARGET}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
        if (just_add_LINK)
            message(NOTICE "just_add_library ${TARGET}: linking to ${just_add_LINK}")
            target_link_libraries("${TARGET}" PUBLIC ${just_add_LINK})
        endif()
    endif()
    if (just_add_DEPENDS)
        message(NOTICE "just_add_library ${TARGET}: adding dependency ${just_add_LINK}")
        add_dependencies("${TARGET}" ${just_add_DEPENDS})
    endif()
    if(NOT is_header_only)
        target_code_coverage(${TARGET} AUTO EXTERNAL ALL EXCLUDE ".conan/.*" ".*/test/.*")
    endif()

    if(NOT "${just_add_SKIP_SUBDIRECTORIES}")
        _just_add_subdirectory()
    endif()
    _just_add_resource("${CMAKE_CURRENT_SOURCE_DIR}")

    if(NOT "${just_add_SKIP_DOXYGEN}")
        _just_add_doxygen(TARGET ${TARGET} FILES ${files})
    endif()
    
    install(TARGETS ${TARGET} EXPORT ${TARGET})
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endfunction()

include(FindDoxygen)
set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY just_doc_counter 0)
set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY just_doc_files)
function(_just_add_doxygen)
    set(oneValueArgs TARGET)
    set(multiValueArgs FILES)
    cmake_parse_arguments(PARSE_ARGV "0" "just_doc" "" "${oneValueArgs}" "${multiValueArgs}")
    # 1. doxygen module support to generate for a one target
    # 2. there is no merge for multiple generation e.g. to merge generation of two libs
    # 3. I dont want the user to invoke a command at the end

    # => add per invocation a new target which contains source files of current and recent invocations + only this is part of all target

    get_property(just_doc_files DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY just_doc_files)
    list(APPEND just_doc_files "${just_doc_FILES}")

    get_property(just_doc_counter DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY just_doc_counter)
    if (${just_doc_counter} GREATER 0)
        set_target_properties("${CMAKE_PROJECT_NAME}-doc-${just_doc_counter}" PROPERTIES EXCLUDE_FROM_ALL TRUE)
    else()
        if (DOXYGEN_USE_MDFILE_AS_MAINPAGE) 
            list(APPEND just_doc_files "${DOXYGEN_USE_MDFILE_AS_MAINPAGE}")
        endif()
    endif()

    math(EXPR just_doc_counter "${just_doc_counter} + 1")

    if(NOT DOXYGEN_OUTPUT_DIRECTORY)
        set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    endif()
    doxygen_add_docs("${CMAKE_PROJECT_NAME}-doc-${just_doc_counter}" "${just_doc_files}" ALL USE_STAMP_FILE WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

    set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY just_doc_counter ${just_doc_counter})
    set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY just_doc_files ${just_doc_files})
endfunction()

function(just_add_executable)
    # Argument parsing
    set(options SKIP_SUBDIRECTORIES)
    set(oneValueArgs SUFFIX)
    set(multiValueArgs INCLUDE EXCLUDE LINK DEPENDS)
    cmake_parse_arguments(PARSE_ARGV "0" "just_add" "${options}" "${oneValueArgs}" "${multiValueArgs}")

    # get name from
    get_filename_component(TARGET "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
    set(TARGET "${CMAKE_PROJECT_NAME}-${TARGET}${just_add_SUFFIX}")
    if(just_add_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "target \"${TARGET}\" has unparsed arguments \"${just_add_UNPARSED_ARGUMENTS}\" maybe LINK in front of it?")
    endif()
    _just_add_check("${TARGET}")

    if (just_add_LINK)
        _just_check_library_order(TARGET "${TARGET}" LINK ${just_add_LINK})
    endif()

    # create filelist
    file(GLOB_RECURSE files include/* src/*)
    if(just_add_INCLUDE)
        foreach(pattern ${just_add_INCLUDE})
            set(just_add_temp ${files})
            list(FILTER just_add_temp INCLUDE REGEX ${pattern})
            list(APPEND all_includes ${just_add_temp})
        endforeach()
        set(files ${all_includes})
    endif()
    if(just_add_EXCLUDE)
        foreach(pattern ${just_add_EXCLUDE})
            list(FILTER files EXCLUDE REGEX ${pattern})
        endforeach()
    endif()

    # create target
    add_executable("${TARGET}" ${files})
    string(TOUPPER "RUNTIME_OUTPUT_DIRECTORY_${CMAKE_BUILD_TYPE}" binary_folder)
    set_target_properties("${TARGET}" PROPERTIES "${binary_folder}" "${CMAKE_CURRENT_BINARY_DIR}")

    target_include_directories("${TARGET}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
    if (just_add_LINK)
        message(NOTICE "just_add_executable ${TARGET}: linking to ${just_add_LINK}")
        target_link_libraries("${TARGET}" PUBLIC ${just_add_LINK})
    endif()
    if (just_add_DEPENDS)
        message(NOTICE "just_add_executable ${TARGET}: adding dependency ${just_add_LINK}")
        add_dependencies("${TARGET}" ${just_add_DEPENDS})
    endif()
    # failing and probably not needed if not lib or test
    # target_code_coverage(${TARGET} AUTO EXTERNAL ALL EXCLUDE ".conan/.*" ".*/test/.*")
    if(NOT "${just_add_SKIP_SUBDIRECTORIES}")
        _just_add_subdirectory()
    endif()

    _just_add_resource("${CMAKE_CURRENT_SOURCE_DIR}")

    install(TARGETS ${TARGET} EXPORT ${TARGET})
endfunction()

function(just_add_tests)
    # Argument parsing
    set(options SKIP_SUBDIRECTORIES SKIP_DEFAULT_LINK)
    set(oneValueArgs SUFFIX)
    set(multiValueArgs INCLUDE EXCLUDE LINK DEPENDS)
    cmake_parse_arguments(PARSE_ARGV "0" "just_add" "${options}" "${oneValueArgs}" "${multiValueArgs}")

    get_filename_component(TARGET_UNDER_TEST "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
    get_filename_component(TARGET_UNDER_TEST "${TARGET_UNDER_TEST}" NAME)
    set(TARGET_UNDER_TEST "${CMAKE_PROJECT_NAME}-${TARGET_UNDER_TEST}")

    # get name from
    get_filename_component(TEST_TARGET "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
    set(TARGET "${TARGET_UNDER_TEST}-${TEST_TARGET}${just_add_SUFFIX}")

    if (just_add_LINK)
        _just_check_library_order(TARGET "${TARGET}" LINK ${just_add_LINK})
    endif()

    if (NOT TARGET "${TARGET_UNDER_TEST}")
        message(FATAL_ERROR "just_add_tests: Expected to create tests for target ${TARGET_UNDER_TEST} but this target doesn't exists.")
    endif()
    if(just_add_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "target \"${TARGET}\" has unparsed arguments \"${just_add_UNPARSED_ARGUMENTS}\" maybe LINK in front of it?")
    endif()
    _just_add_check("${TARGET}")

    # create filelist
    file(GLOB_RECURSE files include/* src/*)
    if(just_add_INCLUDE)
        foreach(pattern ${just_add_INCLUDE})
            set(just_add_temp ${files})
            list(FILTER just_add_temp INCLUDE REGEX ${pattern})
            list(APPEND all_includes ${just_add_temp})
        endforeach()
        set(files ${all_includes})
    endif()
    if(just_add_EXCLUDE)
        foreach(pattern ${just_add_EXCLUDE})
            list(FILTER files EXCLUDE REGEX ${pattern})
        endforeach()
    endif()

    # create target
    add_executable("${TARGET}" ${files})
    string(TOUPPER "RUNTIME_OUTPUT_DIRECTORY_${CMAKE_BUILD_TYPE}" binary_folder)
    set_target_properties("${TARGET}" PROPERTIES "${binary_folder}" "${CMAKE_CURRENT_BINARY_DIR}")
    gtest_discover_tests("${TARGET}" WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" DISCOVERY_TIMEOUT 60 PROPERTIES LABELS ${TARGET_UNDER_TEST})
    target_include_directories("${TARGET}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

    set(LINK_DEPENDENCIES)
    if(NOT "${just_add_SKIP_DEFAULT_LINK}")
        list(APPEND LINK_DEPENDENCIES ${TARGET_UNDER_TEST} ${CONAN_LIBS_GTEST} pthread)
    endif()
    list(APPEND LINK_DEPENDENCIES ${just_add_LINK})

    message(NOTICE "just_add_tests ${TARGET}: linking to ${LINK_DEPENDENCIES}")
    target_link_libraries("${TARGET}" PUBLIC ${LINK_DEPENDENCIES})
    target_code_coverage("${TARGET}" AUTO EXTERNAL ALL EXCLUDE ".conan/.*" ".*/test/.*")

    if (just_add_DEPENDS)
        message(NOTICE "just_add_tests ${TARGET}: adding dependency ${just_add_LINK}")
        add_dependencies("${TARGET}" ${just_add_DEPENDS})
    endif()

    if(NOT "${just_add_SKIP_SUBDIRECTORIES}")
        _just_add_subdirectory()
    endif()

    # if target under tests has resources, the tests need them as well
    get_target_property(target_under_test_source_dir "${TARGET_UNDER_TEST}" "SOURCE_DIR")
    _just_add_resource("${target_under_test_source_dir}")
    # but also allow them to overwrite & have other dependencies too
    _just_add_resource("${CMAKE_CURRENT_SOURCE_DIR}")
endfunction()
