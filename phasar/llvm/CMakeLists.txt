just_add_library(
    ${OPTION_TESTS_DISABLED}
    ${OPTION_DOXYGEN_DISABLED}
    LINK

      LLVMPasses
      LLVMCoroutines
      LLVMipo
      LLVMFrontendOpenMP
      LLVMIRReader
      LLVMAsmParser
      LLVMInstrumentation
      LLVMLinker
      LLVMVectorize
      LLVMObjCARCOpts
      LLVMBitWriter
      LLVMScalarOpts
      LLVMAggressiveInstCombine
      LLVMInstCombine
      LLVMTransformUtils
      LLVMTarget
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
      LLVMSupport
      LLVMDemangle
      sqlite3
      nlohmann_json_schema_validator
      z
      m
      dl
      rt

      phasar-config
      phasar-utils
      phasar-db
      
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
