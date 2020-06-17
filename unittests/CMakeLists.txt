add_custom_target(PhasarUnitTests)
set_target_properties(PhasarUnitTests PROPERTIES FOLDER "Unittests")
add_dependencies(PhasarUnitTests LLFileGeneration)

set(PHASAR_UNITTEST_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(check-phasar-unittests
  COMMAND ${CMAKE_CTEST_COMMAND} --progress --output-on-failure -j 8
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

add_subdirectory(Config)
add_subdirectory(Controller)
add_subdirectory(DB)
add_subdirectory(Experimental)
add_subdirectory(Flex)
add_subdirectory(PhasarClang)
add_subdirectory(PhasarLLVM)
add_subdirectory(Utils)
