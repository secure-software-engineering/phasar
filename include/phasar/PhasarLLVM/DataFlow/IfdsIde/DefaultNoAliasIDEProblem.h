/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, Maximilian Huber and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IDENOALIASINFOTABULATIONPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IDENOALIASINFOTABULATIONPROBLEM_H

#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/IfdsIdeProblemMixin.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"

namespace llvm {
class Value;
class Instruction;
class Function;
} // namespace llvm

namespace psr {

namespace detail {
class IDENoAliasDefaultFlowFunctionsImpl {
public:
  using d_t = const llvm::Value *;
  using n_t = const llvm::Instruction *;
  using f_t = const llvm::Function *;
  using FlowFunctionType = FlowFunction<d_t>;
  using FlowFunctionPtrType = typename FlowFunctionType::FlowFunctionPtrType;

  virtual ~IDENoAliasDefaultFlowFunctionsImpl() = default;

  /// True, if the analysis knows this function, either because it is analyzed,
  /// or because we have external information about it.
  [[nodiscard]] virtual bool isFunctionModeled(f_t Fun) const;

  [[nodiscard]] FlowFunctionPtrType getNormalFlowFunctionImpl(n_t Curr,
                                                              n_t /*Succ*/);
  [[nodiscard]] FlowFunctionPtrType getCallFlowFunctionImpl(n_t CallInst,
                                                            f_t CalleeFun);
  [[nodiscard]] FlowFunctionPtrType getRetFlowFunctionImpl(n_t CallSite,
                                                           f_t /*CalleeFun*/,
                                                           n_t ExitInst,
                                                           n_t /*RetSite*/);
  [[nodiscard]] FlowFunctionPtrType
  getCallToRetFlowFunctionImpl(n_t CallSite, n_t /*RetSite*/,
                               llvm::ArrayRef<f_t> /*Callees*/);
};
} // namespace detail

template <typename AnalysisDomainTy>
class DefaultNoAliasIDEProblem
    : public IfdsIdeProblemMixin<AnalysisDomainTy>,
      protected detail::IDENoAliasDefaultFlowFunctionsImpl {
public:
  using IfdsIdeProblemMixin<AnalysisDomainTy>::IfdsIdeProblemMixin;

  using typename IfdsIdeProblemMixin<AnalysisDomainTy>::f_t;
  using typename IfdsIdeProblemMixin<AnalysisDomainTy>::FlowFunctionPtrType;
  using typename IfdsIdeProblemMixin<AnalysisDomainTy>::n_t;

  [[nodiscard]] FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) {
    return getNormalFlowFunctionImpl(Curr, Succ);
  }

  [[nodiscard]] FlowFunctionPtrType getCallFlowFunction(n_t CallInst,
                                                        f_t CalleeFun) {
    return getCallFlowFunctionImpl(CallInst, CalleeFun);
  }

  [[nodiscard]] FlowFunctionPtrType
  getRetFlowFunction(n_t CallSite, f_t CalleeFun, n_t ExitInst, n_t RetSite) {
    return getRetFlowFunctionImpl(CallSite, CalleeFun, ExitInst, RetSite);
  }

  [[nodiscard]] FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           llvm::ArrayRef<f_t> Callees) {
    return getCallToRetFlowFunctionImpl(CallSite, RetSite, Callees);
  }
};

using DefaultNoAliasIFDSProblem =
    DefaultNoAliasIDEProblem<LLVMIFDSAnalysisDomainDefault>;

} // namespace psr

#endif
