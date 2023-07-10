just_add_executable(
    ${OPTION_TESTS_DISABLED}
    ${OPTION_DOXYGEN_DISABLED}
    LINK
    phasar-llvm
    phasar-controller
    phasar-config
)
