file(GLOB lca_files *.swift)

set(SWIFT_COMPILE_IR_FLAGS -emit-ir -suppress-warnings -g -parse-as-library -Onone)

foreach(TEST_SRC ${lca_files})
  get_filename_component(TEST_SRC_FILE ${TEST_SRC} NAME_WE)
  set(TEST_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${TEST_SRC_FILE}.ll")
  add_custom_command(
    OUTPUT ${TEST_OUTPUT}
    COMMAND swiftc ${SWIFT_COMPILE_IR_FLAGS} -o ${TEST_OUTPUT} ${TEST_SRC}
    DEPENDS ${TEST_SRC}
  )
  add_custom_target(${TEST_SRC_FILE} DEPENDS ${TEST_OUTPUT})
  add_dependencies(PhasarUnitTests ${TEST_SRC_FILE})
endforeach(TEST_SRC)
