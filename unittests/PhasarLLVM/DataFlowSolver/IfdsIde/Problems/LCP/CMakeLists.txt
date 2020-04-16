file(GLOB_RECURSE LCP_TEST_SRC *.cpp)

#message("Module-path: ${CMAKE_MODULE_PATH}")
set(LLVM_INDEPENDENT_TESTS
    TypeCheckTest
)

foreach(TEST_SRC ${LCP_TEST_SRC})
    add_ccpp_unittest(${TEST_SRC})
    get_filename_component(test ${TEST_SRC} NAME_WE)
    
    target_link_libraries(${test}
        #FormatConverter
        CCPPLinearConstantPropagation
	    PredicateResolver
        ${llvm_libs}
        ${phasar_libs}
        dl
    )
    
endforeach(TEST_SRC)
