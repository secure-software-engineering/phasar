#include "phasar/PhasarLLVM/AnalysisStrategy/HelperAnalyses.h"
#include "phasar/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include <memory>
#include <string>

namespace psr {
HelperAnalyses::HelperAnalyses(std::string IRFile,
                               std::optional<nlohmann::json> PrecomputedPTS,
                               PointerAnalysisType PTATy, bool AllowLazyPTS,
                               std::vector<std::string> EntryPoints,
                               CallGraphAnalysisType CGTy,
                               Soundness SoundnessLevel,
                               bool AutoGlobalSupport) noexcept
    : IRFile(std::move(IRFile)), PrecomputedPTS(std::move(PrecomputedPTS)),
      PTATy(PTATy), AllowLazyPTS(AllowLazyPTS),
      EntryPoints(std::move(EntryPoints)), CGTy(CGTy),
      SoundnessLevel(SoundnessLevel), AutoGlobalSupport(AutoGlobalSupport) {}

HelperAnalyses::HelperAnalyses(std::string IRFile,
                               std::vector<std::string> EntryPoints,
                               HelperAnalysisConfig Config) noexcept
    : IRFile(std::move(IRFile)),
      PrecomputedPTS(std::move(Config.PrecomputedPTS)), PTATy(Config.PTATy),
      AllowLazyPTS(Config.AllowLazyPTS), EntryPoints(std::move(EntryPoints)),
      CGTy(Config.CGTy), SoundnessLevel(Config.SoundnessLevel),
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

HelperAnalyses::~HelperAnalyses() noexcept = default;

LLVMProjectIRDB &HelperAnalyses::getProjectIRDB() {
  if (!IRDB) {
    IRDB = std::make_unique<LLVMProjectIRDB>(IRFile);
  }
  return *IRDB;
}

LLVMPointsToInfo &HelperAnalyses::getPointsToInfo() {
  if (!PT) {
    if (PrecomputedPTS.has_value()) {
      PT = std::make_unique<LLVMPointsToSet>(getProjectIRDB(), *PrecomputedPTS);
    } else {
      PT = std::make_unique<LLVMPointsToSet>(getProjectIRDB(), AllowLazyPTS,
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
    ICF = std::make_unique<LLVMBasedICFG>(
        &getProjectIRDB(), CGTy, std::move(EntryPoints), &getTypeHierarchy(),
        CGTy == CallGraphAnalysisType::OTF ? &getPointsToInfo() : nullptr,
        SoundnessLevel, AutoGlobalSupport);
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
