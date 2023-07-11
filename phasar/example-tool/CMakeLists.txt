just_add_executable(
  ${OPTION_TESTS_DISABLED}
  LINK
    phasar-all
    pthread # TODO: needed after llvm mr + cmake find package?
)
