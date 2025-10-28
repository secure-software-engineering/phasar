add_custom_target(PhasarUnitTests)
set_target_properties(PhasarUnitTests PROPERTIES FOLDER "Unittests")
add_dependencies(PhasarUnitTests LLFileGeneration)

set(PHASAR_UNITTEST_DIR ${CMAKE_CURRENT_BINARY_DIR})

cmake_host_system_information(RESULT cores QUERY NUMBER_OF_LOGICAL_CORES)
set(additional_args -j ${cores})

# ignore flaky tests
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  execute_process(COMMAND lsb_release -r
                  OUTPUT_VARIABLE UBUNTU_VERSION
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX REPLACE "^[^0-9]+([0-9]+)\\.([0-9]+).*" "\\1" UBUNTU_MAJOR_VERSION "${UBUNTU_VERSION}")

    if ("${UBUNTU_MAJOR_VERSION}" GREATER 22)
      # TODO tests shouldn't be flaky
      list(APPEND additional_args -E "\"(LLVMBasedCFGTest|LLVMBasedICFGGlobCtorDtorTest|IDEInstInteractionAnalysisTest|IFDSUninitializedVariablesTest|IDEGeneralizedLCATest|IDEExtendedTaintAnalysisTest|IDEFeatureTaintAnalysis)\"")
      # LLVMBasedCFGTest.HandlesCppStandardType
      # IDEInstInteractionAnalysisTest.HandleBasicTest_04
      # IFDSUninitializedVariablesTest.UninitTest_05_SHOULD_NOT_LEAK
      #  .UninitTest_06_SHOULD_NOT_LEAK
      # IDEGeneralizedLCATest.StringTestCpp
      # IDEExtendedTaintAnalysisTest.XTaint09
      # IDEFeatureTaintAnalysis.HandleBasicTest_04, HandleGlobalTest_05
    endif()
endif()

add_custom_target(check-phasar-unittests
  COMMAND ${CMAKE_CTEST_COMMAND} --progress --output-on-failure ${additional_args}
  WORKING_DIRECTORY ${PHASAR_UNITTEST_DIR}
  DEPENDS PhasarUnitTests
)

# Provide config files for unit tests
file(MAKE_DIRECTORY ${PHASAR_UNITTEST_DIR}/config/)
set(PHASAR_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../config/")
function(test_require_config_file file_name)
  configure_file("${PHASAR_CONFIG_DIR}${file_name}"
    "${PHASAR_UNITTEST_DIR}/config/."
    COPYONLY
  )
endfunction()

include_directories(TestUtils)

add_subdirectory(PhasarLLVM)
add_subdirectory(Utils)
if(BUILD_PHASAR_CLANG)
  add_subdirectory(PhasarClang)
endif()
