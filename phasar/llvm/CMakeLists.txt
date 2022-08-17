just_add_library(
    ${OPTION_TESTS_DISABLED}
    LINK
      phasar-config
      phasar-utils
      phasar-db

      LLVMAggressiveInstCombine # llvm::createAggressiveInstCombinerPass()
      LLVMAnalysis
      LLVMAsmParser # undefined symbol: llvm::parseAssembly
      LLVMBinaryFormat # llvm::dwarf::getOperationEncoding
      LLVMBitReader # undefined symbol: llvm::getOwningLazyBitcodeModule
      LLVMBitstreamReader # undefined symbol: llvm::BitstreamCursor::skipRecord
      LLVMCore
      LLVMCoroutines # llvm::CoroCleanupPass::run
      LLVMDebugInfoDWARF # llvm::DWARFFormValue::getAsCString()
      LLVMDemangle
      LLVMFrontendOpenMP # llvm::OpenMPIRBuilder::addAttributes
      LLVMIRReader
      LLVMInstCombine # llvm::createInstructionCombiningPass()
      LLVMInstrumentation # llvm::MemorySanitizerOptions::MemorySanitizerOptions
      LLVMLinker
      LLVMMC # llvm::TargetRegistry::lookupTarget
      LLVMMCParser # llvm::MCAsmParser::setTargetParser
      LLVMObjCARCOpts # llvm::ObjCARCExpandPass::run
      LLVMObject # undefined symbol: llvm::ModuleSymbolTable::CollectAsmSymvers
      LLVMPasses
      LLVMProfileData # undefined symbol ProfileSummaryCutoffHot
      LLVMRemarks # llvm::remarks::RemarkStreamer::matchesFilter
      LLVMScalarOpts # llvm::ADCEPass::run
      LLVMSupport
      LLVMTarget # llvm::TargetMachine::getTargetIRAnalysis()
      LLVMTextAPI # llvm::MachO::mapToPlatformSet(llvm::ArrayRef
      LLVMTransformUtils # undefined symbol: llvm::ValueMapper::ValueMapper
      LLVMVectorize # llvm::ShouldRunExtraVectorPasses::Key
      LLVMipo # llvm::AttributorPass::run
      
      sqlite3
      nlohmann_json_schema_validator
      z
    EXCLUDE "^.*/WPDS/.*$" "phasar-llvm.cpp$"
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
