file(GLOB_RECURSE LCA_SRC *.cpp)

add_library(CCPPLinearConstantPropagation SHARED ${LCA_SRC})

target_link_libraries(CCPPLinearConstantPropagation
    Parser
    ${llvm_libs}
    ${phasar_libs}
)
