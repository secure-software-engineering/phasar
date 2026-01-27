#include "phasar/PhasarLLVM/Pointer/AliasAnalysisView.h"

#include "phasar/Config/phasar-config.h" // for PHASAR_USE_SVF
#include "phasar/Pointer/AliasAnalysisType.h"

#include "llvm/Support/ErrorHandling.h"

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
  case AliasAnalysisType::UnionFind:
    llvm::report_fatal_error(
        "Cannot use AliasAnalysisType::UnionFind with LLVMAliasSet. Use "
        "LLVMUnionFindAliasSet instead!");
  default:
    return createLLVMBasedAnalysis(IRDB, UseLazyEvaluation, PATy);
  }
}
