#include "phasar/PhasarLLVM/AnalysisStrategy/HelperAnalyses.h"

#include "phasar/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include <memory>

namespace psr {
HelperAnalyses::HelperAnalyses(std::string IRFile,
                               std::optional<nlohmann::json> PrecomputedPTS,
                               AliasAnalysisType PTATy, bool AllowLazyPTS,
                               std::vector<std::string> EntryPoints,
                               CallGraphAnalysisType CGTy,
                               Soundness SoundnessLevel, bool AutoGlobalSupport)
    : IRFile(std::move(IRFile)), PrecomputedPTS(std::move(PrecomputedPTS)),
      PTATy(PTATy), AllowLazyPTS(AllowLazyPTS),
      EntryPoints(std::move(EntryPoints)), CGTy(CGTy),
      SoundnessLevel(SoundnessLevel), AutoGlobalSupport(AutoGlobalSupport) {}

HelperAnalyses::HelperAnalyses(std::string IRFile,
                               std::vector<std::string> EntryPoints,
                               HelperAnalysisConfig Config)
    : IRFile(std::move(IRFile)),
      PrecomputedPTS(std::move(Config.PrecomputedPTS)), PTATy(Config.PTATy),
      AllowLazyPTS(Config.AllowLazyPTS), EntryPoints(std::move(EntryPoints)),
      CGTy(Config.CGTy), SoundnessLevel(Config.SoundnessLevel),
      AutoGlobalSupport(Config.AutoGlobalSupport) {}

HelperAnalyses::~HelperAnalyses() = default;

LLVMProjectIRDB &HelperAnalyses::getProjectIRDB() {
  if (!IRDB) {
    IRDB = std::make_unique<LLVMProjectIRDB>(IRFile);
  }
  return *IRDB;
}

LLVMAliasSet &HelperAnalyses::getPointsToInfo() {
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
    ICF = std::make_unique<LLVMBasedICFG>(
        &getProjectIRDB(), CGTy, std::move(EntryPoints), &getTypeHierarchy(),
        CGTy == CallGraphAnalysisType::OTF ? &getPointsToInfo() : nullptr,
        SoundnessLevel, AutoGlobalSupport);
  }

  return *ICF;
}

} // namespace psr
