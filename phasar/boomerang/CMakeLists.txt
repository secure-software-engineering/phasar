just_add_executable(
  ${OPTION_TESTS_DISABLED}
  LINK
    phasar-llvm
    phasar-controller
    pthread
)
