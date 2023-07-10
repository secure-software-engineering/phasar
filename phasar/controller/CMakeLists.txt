just_add_library(
    ${OPTION_TESTS_DISABLED}
    ${OPTION_DOXYGEN_DISABLED}
    LINK
        phasar-llvm
        phasar-analysisstrategy
        phasar-dataflow
)
