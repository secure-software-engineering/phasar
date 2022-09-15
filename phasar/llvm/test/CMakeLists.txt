just_add_tests(
    LINK
        LLVMCore
        LLVMRemarks
        phasar-test-utils
    DEPENDS
        LLFileGeneration
    EXCLUDE "^.*/WPDS/.*$" #removed wali
)
