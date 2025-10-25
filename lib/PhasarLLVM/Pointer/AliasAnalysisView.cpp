#include "phasar/PhasarLLVM/Pointer/AliasAnalysisView.h"

#include "phasar/Config/phasar-config.h" // for PHASAR_USE_SVF

#ifdef PHASAR_USE_SVF
#include "SVF/SVFBasedAliasAnalysis.h"
#endif

#include <memory>

using namespace psr;

std::unique_ptr<AliasAnalysisView>
AliasAnalysisView::create(LLVMProjectIRDB &IRDB, bool UseLazyEvaluation,
                          AliasAnalysisType PATy) {
  switch (PATy) {
#ifdef PHASAR_USE_SVF
  case AliasAnalysisType::SVFDDA:
    return createSVFDDAAnalysis(IRDB);

  case AliasAnalysisType::SVFVFS:
    return createSVFVFSAnalysis(IRDB);
#endif
  default:
    return createLLVMBasedAnalysis(IRDB, UseLazyEvaluation, PATy);
  }
}
