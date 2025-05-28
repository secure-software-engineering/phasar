file(GLOB_RECURSE PSR_LLVM_DOMAIN_SRC *.h *.cpp)

add_phasar_library(phasar_llvm_domain
  ${PSR_LLVM_DOMAIN_SRC}

  MODULE_FILES
    Domain.cppm
)
