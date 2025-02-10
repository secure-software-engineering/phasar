#include "phasar/PhasarLLVM/Pointer/AliasAnalysisView.h"

#include "phasar/Config/phasar-config.h"

#ifdef PHASAR_USE_SVF
#include "SVF/SVFBasedAliasAnalysis.h"
#endif

#include <memory>

using namespace psr;

std::unique_ptr<AliasAnalysisView>
AliasAnalysisView::create(LLVMProjectIRDB &IRDB, bool UseLazyEvaluation,
                          AliasAnalysisType PATy) {
  switch (PATy) {
  case AliasAnalysisType::SVFDDA:
#ifndef PHASAR_USE_SVF
    throw std::runtime_error("AliasAnalysisType::SVFVFS requires SVF, which is "
                             "not included in your PhASAR build!");
#else
    return createSVFDDAAnalysis(IRDB);
#endif
  case AliasAnalysisType::SVFVFS:
#ifndef PHASAR_USE_SVF
    throw std::runtime_error("AliasAnalysisType::SVFDDA requires SVF, which is "
                             "not included in your PhASAR build!");
#else
    return createSVFVFSAnalysis(IRDB);
#endif
  default:
    return createLLVMBasedAnalysis(IRDB, UseLazyEvaluation, PATy);
  }
}
