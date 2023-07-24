just_add_tests(
    LINK
        LLVMCore
        LLVMRemarks
        phasar-test-utils
    DEPENDS
        LLFileGeneration

    # TODO: fix conan integration
    EXCLUDE
        test/src/DataFlow/IfdsIde/Problems/IDELinearConstantAnalysisSwiftTest.cpp
)
