#include "phasar/PhasarLLVM/AnalysisStrategy/HelperAnalyses.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include <memory>

namespace psr {
HelperAnalyses::HelperAnalyses(std::vector<std::string> IRFiles,
                               std::optional<nlohmann::json> PrecomputedPTS,
                               PointerAnalysisType PTATy, bool AllowLazyPTS,
                               std::vector<std::string> EntryPoints,
                               CallGraphAnalysisType CGTy,
                               Soundness SoundnessLevel, bool AutoGlobalSupport)
    : IRFiles(std::move(IRFiles)), PrecomputedPTS(std::move(PrecomputedPTS)),
      PTATy(PTATy), AllowLazyPTS(AllowLazyPTS),
      EntryPoints(std::move(EntryPoints)), CGTy(CGTy),
      SoundnessLevel(SoundnessLevel), AutoGlobalSupport(AutoGlobalSupport) {}

HelperAnalyses::HelperAnalyses(std::vector<std::string> IRFiles,
                               std::vector<std::string> EntryPoints,
                               HelperAnalysisConfig Config)
    : IRFiles(std::move(IRFiles)),
      PrecomputedPTS(std::move(Config.PrecomputedPTS)), PTATy(Config.PTATy),
      AllowLazyPTS(Config.AllowLazyPTS), EntryPoints(std::move(EntryPoints)),
      CGTy(Config.CGTy), SoundnessLevel(Config.SoundnessLevel),
      AutoGlobalSupport(Config.AutoGlobalSupport) {}

HelperAnalyses::~HelperAnalyses() = default;

ProjectIRDB &HelperAnalyses::getProjectIRDB() {
  if (!IRDB) {
    IRDB = std::make_unique<ProjectIRDB>(IRFiles);
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

} // namespace psr
