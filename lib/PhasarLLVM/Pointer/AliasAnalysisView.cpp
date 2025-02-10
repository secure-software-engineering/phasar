#include "phasar/PhasarLLVM/Pointer/AliasAnalysisView.h"

#include "LLVMBasedAliasAnalysis.h"
#include "SVF/SVFBasedAliasAnalysis.h"

#include <memory>

using namespace psr;

std::unique_ptr<AliasAnalysisView>
AliasAnalysisView::create(LLVMProjectIRDB &IRDB, bool UseLazyEvaluation,
                          AliasAnalysisType PATy) {
  switch (PATy) {
  case AliasAnalysisType::SVFDDA:
    return createSVFDDAAnalysis(IRDB);
  case AliasAnalysisType::SVFVFS:
    return createSVFVFSAnalysis(IRDB);
  default:
    return std::make_unique<LLVMBasedAliasAnalysis>(IRDB, UseLazyEvaluation,
                                                    PATy);
  }
}
