if(PHASAR_USE_Z3)
    add_phasar_unittest(PathTracingTest.cpp)

    target_link_libraries(PathTracingTest
        LINK_PUBLIC
        phasar_llvm_pathsensitivity
        libz3 # -- the Z3 cmake target is named libz3, so we want to have it as dependency, not just as link-target
    )
endif()
