just_add_library(
    LINK
        LLVMCore
        LLVMOption
        LLVMSupport
        
        clangAST
        clangBasic
        clangFrontend
        clangLex
        clangRewrite
        omp
        
        clangSerialization
        clangTooling
        phasar-utils
    EXCLUDE "phasar-clang.cpp$"
)
# openmp needed: llvm::omp::getOpenMPDirectiveName

just_add_executable(
    LINK
        phasar-llvm
        phasar-clang
        pthread
    INCLUDE "phasar-clang.cpp$"

    SUFFIX "-tool"
    SKIP_SUBDIRECTORIES
)
