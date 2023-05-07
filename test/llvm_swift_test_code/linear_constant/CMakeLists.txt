file(GLOB lca_files *.swift)

set(SWIFT_COMPILE_IR_FLAGS -emit-ir -suppress-warnings -g -parse-as-library -Onone -Xfrontend -disable-llvm-optzns -Xfrontend -disable-swift-specific-llvm-optzns)

foreach(TEST_SRC ${lca_files})
  get_filename_component(TEST_SRC_FILE ${TEST_SRC} NAME_WE)
  add_executable(${TEST_SRC_FILE}.ll ${TEST_SRC})
  target_compile_options(${TEST_SRC_FILE}.ll PRIVATE ${SWIFT_COMPILE_IR_FLAGS})
endforeach(TEST_SRC)
