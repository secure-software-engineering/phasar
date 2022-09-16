just_add_library(
    ${OPTION_TESTS_DISABLED}
    ${OPTION_DOXYGEN_DISABLED}
    LINK 
        LLVMBitWriter
        LLVMCore
        LLVMSupport
        LLVMDemangle
        
        phasar-config
  )
