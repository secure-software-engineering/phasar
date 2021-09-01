#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/XTaintAnalysisBase.h"
#include "phasar/PhasarLLVM/TaintConfig/TaintConfig.h"

namespace psr::XTaint {
AnalysisBase::AnalysisBase(const TaintConfig *TSF) noexcept : TSF(TSF) {
  assert(TSF != nullptr);
}

auto AnalysisBase::getConfigurationAt(const llvm::Instruction *Inst,
                                      const llvm::Function *Callee) const
    -> std::pair<SourceConfigTy, SinkConfigTy> {
  return {getSourceConfigAt(Inst, Callee), getSinkConfigAt(Inst, Callee)};
}

auto AnalysisBase::getSourceConfigAt(const llvm::Instruction *Inst,
                                     const llvm::Function *Callee) const
    -> SourceConfigTy {
  SourceConfigTy ret;

  TSF->forAllGeneratedValuesAt(Inst, Callee,
                               [&ret](const llvm::Value *V) { ret.insert(V); });

  return ret;
}

auto AnalysisBase::getSinkConfigAt(const llvm::Instruction *Inst,
                                   const llvm::Function *Callee) const
    -> SinkConfigTy {
  SinkConfigTy ret;

  TSF->forAllLeakCandidatesAt(Inst, Callee,
                              [&ret](const llvm::Value *V) { ret.insert(V); });

  return ret;
}

auto AnalysisBase::getSanitizerConfigAt(const llvm::Instruction *Inst,
                                        const llvm::Function *Callee) const
    -> SanitizerConfigTy {
  SanitizerConfigTy ret;

  TSF->forAllSanitizedValuesAt(Inst, Callee,
                               [&ret](const llvm::Value *V) { ret.insert(V); });

  return ret;
}
} // namespace psr::XTaint