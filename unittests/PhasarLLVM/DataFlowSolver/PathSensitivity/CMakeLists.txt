add_phasar_unittest(PathTracingTest.cpp)

target_link_libraries(PathTracingTest
    LINK_PUBLIC
    phasar_pathsensitivity
    z3
)
