file(GLOB lca_files *.c *.cpp)

foreach(TEST_SRC ${lca_files})
  get_filename_component(TEST_SRC_FILE ${TEST_SRC} NAME)
  generate_ll_file(FILE ${TEST_SRC_FILE} DEBUG)
endforeach(TEST_SRC)

foreach(TEST_SRC ${lca_files})
  get_filename_component(TEST_SRC_FILE ${TEST_SRC} NAME)
  generate_ll_file(FILE ${TEST_SRC_FILE})
endforeach(TEST_SRC)

file(GLOB lca_ir_files *.ll *.bc)

foreach(TEST_IR ${lca_ir_files})
  message(STATUS "Directly copying ${TEST_IR} into ${CMAKE_CURRENT_BINARY_DIR}")
  configure_file("${TEST_IR}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
endforeach(TEST_IR)
