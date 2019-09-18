# copied from phasar/cmake/phasar_macros.cmake and modified a bit
function(add_ccpp_unittest test_name)
  message("Set-up unittest: ${test_name}")
  get_filename_component(test ${test_name} NAME_WE)
  add_executable(${test}
    ${test_name}
  )
  
  
  target_link_libraries(${test}
    Parser
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    gtest
    boost_log
    boost_system
    stdc++fs
  )

  add_test(NAME "${test}"
    COMMAND ${test} ${CATCH_TEST_FILTER}
  )
  set_tests_properties("${test}" PROPERTIES LABELS "all")
  set(CTEST_OUTPUT_ON_FAILURE ON)
endfunction()