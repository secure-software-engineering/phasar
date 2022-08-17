just_add_library(
    ${OPTION_TESTS_DISABLED}
    LINK 
        LLVMBinaryFormat
        LLVMBitWriter
        LLVMBitstreamReader
        LLVMCore
        LLVMDemangle
        LLVMRemarks
        LLVMSupport
        
        phasar-config
  )
