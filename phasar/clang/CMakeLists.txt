just_add_library(
    ${OPTION_TESTS_DISABLED}
    ${OPTION_DOXYGEN_DISABLED}
    LINK
        clangTooling # core dep
        clangFrontend # core dep
        clangDriver
        clangParse
        clangSerialization
        clangSema
        clangAnalysis
        clangASTMatchers
        clangEdit
        clangAST
        clangFormat
        clangToolingInclusions
        clangToolingCore
        clangRewrite # core dep
        clangLex
        clangBasic
        LLVMFrontendOpenMP
        LLVMScalarOpts
        LLVMAggressiveInstCombine
        LLVMInstCombine
        LLVMTransformUtils
        LLVMAnalysis
        LLVMProfileData
        LLVMDebugInfoDWARF
        LLVMObject
        LLVMMCParser
        LLVMMC
        LLVMDebugInfoCodeView
        LLVMBitReader
        LLVMCore
        LLVMRemarks
        LLVMBitstreamReader
        LLVMTextAPI
        LLVMBinaryFormat
        LLVMOption
        LLVMSupport
        LLVMDemangle
        rt
        dl
        m
        pthread
        z



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
