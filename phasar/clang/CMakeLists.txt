just_add_library(
    ${OPTION_TESTS_DISABLED}
    ${OPTION_DOXYGEN_DISABLED}
    LINK
        LLVMCore
        LLVMOption
        LLVMSupport
        LLVMProfileData
        LLVMObject
        LLVMDebugInfoDWARF
        LLVMMCParser
        LLVMMC
        LLVMBitReader
        LLVMTextAPI
        
        clangTooling
        clangFrontendTool
        clangFrontend
        clangDriver
        clangSerialization
        clangCodeGen
        clangParse
        clangSema
        clangStaticAnalyzerFrontend
        clangStaticAnalyzerCheckers
        clangStaticAnalyzerCore
        clangAnalysis
        clangARCMigrate
        clangRewrite
        clangRewriteFrontend
        clangEdit
        clangAST
        clangASTMatchers
        clangLex
        clangBasic
        LLVMFrontendOpenMP
        dl

        phasar-utils
    EXCLUDE "phasar-clang.cpp$"
)

just_add_executable(
    LINK
        phasar-llvm
        phasar-clang
        pthread
    INCLUDE "phasar-clang.cpp$"

    SUFFIX "-tool"
    SKIP_SUBDIRECTORIES
)
