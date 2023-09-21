#include "phasar/PhasarLLVM/HelperAnalyses.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include <memory>
#include <string>

namespace psr {
HelperAnalyses::HelperAnalyses(std::string IRFile,
                               std::optional<nlohmann::json> PrecomputedPTS,
                               AliasAnalysisType PTATy, bool AllowLazyPTS,
                               std::vector<std::string> EntryPoints,
                               std::optional<nlohmann::json> PrecomputedCG,
                               CallGraphAnalysisType CGTy,
                               Soundness SoundnessLevel,
                               bool AutoGlobalSupport) noexcept
    : IRFile(std::move(IRFile)), PrecomputedPTS(std::move(PrecomputedPTS)),
      PTATy(PTATy), AllowLazyPTS(AllowLazyPTS),
      PrecomputedCG(std::move(PrecomputedCG)),
      EntryPoints(std::move(EntryPoints)), CGTy(CGTy),
      SoundnessLevel(SoundnessLevel), AutoGlobalSupport(AutoGlobalSupport) {}

HelperAnalyses::HelperAnalyses(std::string IRFile,
                               std::vector<std::string> EntryPoints,
                               HelperAnalysisConfig Config) noexcept
    : IRFile(std::move(IRFile)),
      PrecomputedPTS(std::move(Config.PrecomputedPTS)), PTATy(Config.PTATy),
      AllowLazyPTS(Config.AllowLazyPTS),
      PrecomputedCG(std::move(Config.PrecomputedCG)),
      EntryPoints(std::move(EntryPoints)), CGTy(Config.CGTy),
      SoundnessLevel(Config.SoundnessLevel),
      AutoGlobalSupport(Config.AutoGlobalSupport) {}

HelperAnalyses::HelperAnalyses(const llvm::Twine &IRFile,
                               std::vector<std::string> EntryPoints,
                               HelperAnalysisConfig Config)
    : HelperAnalyses(IRFile.str(), std::move(EntryPoints), std::move(Config)) {}
HelperAnalyses::HelperAnalyses(const char *IRFile,
                               std::vector<std::string> EntryPoints,
                               HelperAnalysisConfig Config)
    : HelperAnalyses(std::string(IRFile), std::move(EntryPoints),
                     std::move(Config)) {}
HelperAnalyses::HelperAnalyses(llvm::Module *IRModule,
                               std::vector<std::string> EntryPoints,
                               HelperAnalysisConfig Config)
    : HelperAnalyses(std::string(), std::move(EntryPoints), std::move(Config)) {
  this->IRDB = std::make_unique<LLVMProjectIRDB>(
      IRModule, Config.PreprocessExistingModule);
}
HelperAnalyses::HelperAnalyses(std::unique_ptr<llvm::Module> IRModule,
                               std::vector<std::string> EntryPoints,
                               HelperAnalysisConfig Config)
    : HelperAnalyses(std::string(), std::move(EntryPoints), std::move(Config)) {
  this->IRDB = std::make_unique<LLVMProjectIRDB>(
      std::move(IRModule), Config.PreprocessExistingModule);
}

HelperAnalyses::~HelperAnalyses() noexcept = default;

LLVMProjectIRDB &HelperAnalyses::getProjectIRDB() {
  if (!IRDB) {
    IRDB = std::make_unique<LLVMProjectIRDB>(IRFile);
  }
  return *IRDB;
}

LLVMAliasSet &HelperAnalyses::getAliasInfo() {
  if (!PT) {
    if (PrecomputedPTS.has_value()) {
      PT = std::make_unique<LLVMAliasSet>(&getProjectIRDB(), *PrecomputedPTS);
    } else {
      PT = std::make_unique<LLVMAliasSet>(&getProjectIRDB(), AllowLazyPTS,
                                          PTATy);
    }
  }
  return *PT;
}

LLVMTypeHierarchy &HelperAnalyses::getTypeHierarchy() {
  if (!TH) {
    TH = std::make_unique<LLVMTypeHierarchy>(getProjectIRDB());
  }
  return *TH;
}

LLVMBasedICFG &HelperAnalyses::getICFG() {
  if (!ICF) {
    if (PrecomputedCG.has_value()) {
      ICF = std::make_unique<LLVMBasedICFG>(&getProjectIRDB(), *PrecomputedCG);
    } else {
      ICF = std::make_unique<LLVMBasedICFG>(
          &getProjectIRDB(), CGTy, std::move(EntryPoints), &getTypeHierarchy(),
          CGTy == CallGraphAnalysisType::OTF ? &getAliasInfo() : nullptr,
          SoundnessLevel, AutoGlobalSupport);
    }
  }

  return *ICF;
}

LLVMBasedCFG &HelperAnalyses::getCFG() {
  if (!CFG) {
    if (ICF) {
      return *ICF;
    }
    CFG = std::make_unique<LLVMBasedCFG>();
  }
  return *CFG;
}

} // namespace psr
