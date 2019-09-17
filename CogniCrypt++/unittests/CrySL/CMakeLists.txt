file(GLOB_RECURSE CRYSL_PARSER_TEST_SRC *.h *.cpp)

message("Module-path: ${CMAKE_MODULE_PATH}")

foreach(TEST_SRC ${CRYSL_PARSER_TEST_SRC})
    add_ccpp_unittest(${TEST_SRC})
endforeach(TEST_SRC)