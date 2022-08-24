just_add_library(
    ${OPTION_TESTS_DISABLED}
    ${OPTION_DOXYGEN_DISABLED}
    LINK
      phasar-config
      phasar-utils
      phasar-db

      LLVMAggressiveInstCombine
      LLVMAnalysis
      LLVMAsmParser
      LLVMBinaryFormat
      LLVMBitReader
      LLVMBitstreamReader
      LLVMCore
      LLVMCoroutines
      LLVMDebugInfoDWARF
      LLVMDemangle
      LLVMFrontendOpenMP
      LLVMIRReader
      LLVMInstCombine
      LLVMInstrumentation
      LLVMLinker
      LLVMMC
      LLVMMCParser
      LLVMObjCARCOpts
      LLVMObject
      LLVMPasses
      LLVMProfileData
      LLVMRemarks
      LLVMScalarOpts
      LLVMSupport
      LLVMTarget
      LLVMTextAPI
      LLVMTransformUtils
      LLVMVectorize
      LLVMipo
      
      sqlite3
      nlohmann_json_schema_validator
    EXCLUDE "phasar-llvm.cpp$" "^.*/WPDS/.*$" #removed wali
)

just_add_executable(
  LINK
    phasar-llvm
    phasar-controller
    pthread
  INCLUDE "phasar-llvm.cpp$"
  SUFFIX "-tool"
  SKIP_SUBDIRECTORIES
)
