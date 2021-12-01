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
  SourceConfigTy Ret;

  TSF->forAllGeneratedValuesAt(Inst, Callee,
                               [&Ret](const llvm::Value *V) { Ret.insert(V); });

  return Ret;
}

auto AnalysisBase::getSinkConfigAt(const llvm::Instruction *Inst,
                                   const llvm::Function *Callee) const
    -> SinkConfigTy {
  SinkConfigTy Ret;

  TSF->forAllLeakCandidatesAt(Inst, Callee,
                              [&Ret](const llvm::Value *V) { Ret.insert(V); });

  return Ret;
}

auto AnalysisBase::getSanitizerConfigAt(const llvm::Instruction *Inst,
                                        const llvm::Function *Callee) const
    -> SanitizerConfigTy {
  SanitizerConfigTy Ret;

  TSF->forAllSanitizedValuesAt(Inst, Callee,
                               [&Ret](const llvm::Value *V) { Ret.insert(V); });

  return Ret;
}

bool AnalysisBase::isSink(const llvm::Value *SinkCandidate,
                          const llvm::Instruction *AtInst) const {
  if (TSF->isSink(SinkCandidate)) {
    return true;
  }

  if (!AtInst) {
    return false;
  }

  if (const auto &SinkCB = TSF->getRegisteredSinkCallBack();
      SinkCB && SinkCB(AtInst).count(SinkCandidate)) {
    return true;
  }

  return false;
}
} // namespace psr::XTaint
