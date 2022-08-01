just_add_library(
    LINK
        LLVMCore
        LLVMOption
        LLVMSupport
        #clang-cpp
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
