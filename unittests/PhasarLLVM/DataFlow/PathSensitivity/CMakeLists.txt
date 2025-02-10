if(PHASAR_USE_Z3)
    add_phasar_unittest(PathTracingTest.cpp)

    target_link_libraries(PathTracingTest
        LINK_PUBLIC
        phasar_llvm_pathsensitivity
        ${Z3_LIBRARIES}
    )
endif()
