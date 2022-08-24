just_add_tests(
    LINK
        phasar-test-utils
    DEPENDS
        LLFileGeneration
    EXCLUDE "^.*/WPDS/.*$" #removed wali
)
