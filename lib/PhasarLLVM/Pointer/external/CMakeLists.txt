file(GLOB_RECURSE LLVM_AA_SRC *.h *.cpp)
target_sources(phasar_llvm_pointer PRIVATE ${LLVM_AA_SRC})
