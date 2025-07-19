file(GLOB_RECURSE SVF_AA_SRC *.h *.cpp)

add_phasar_library(phasar_llvm_pointer_svf
  ${SVF_AA_SRC}

  LINKS
    phasar_utils
    phasar_pointer
    phasar_llvm_utils
    phasar_llvm_db

  LLVM_LINK_COMPONENTS
    Core
    Support
    Analysis
    Passes
    Demangle

  LINK_PRIVATE
    SvfLLVM SvfCore ${Z3_LIBRARIES}
)

target_include_directories(phasar_llvm_pointer_svf SYSTEM PRIVATE ${SVF_INSTALL_INCLUDE_DIR})
target_link_directories(phasar_llvm_pointer_svf PUBLIC ${SVF_INSTALL_LIB_DIR})
