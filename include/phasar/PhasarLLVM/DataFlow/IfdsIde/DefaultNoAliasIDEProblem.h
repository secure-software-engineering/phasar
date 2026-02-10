/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, mxHuber and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IDENOALIASINFOTABULATIONPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IDENOALIASINFOTABULATIONPROBLEM_H

#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
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
    : public IDETabulationProblem<AnalysisDomainTy>,
      protected detail::IDENoAliasDefaultFlowFunctionsImpl {
public:
  using IDETabulationProblem<AnalysisDomainTy>::IDETabulationProblem;

  using typename IDETabulationProblem<AnalysisDomainTy>::f_t;
  using typename IDETabulationProblem<AnalysisDomainTy>::FlowFunctionPtrType;
  using typename IDETabulationProblem<AnalysisDomainTy>::n_t;

  [[nodiscard]] FlowFunctionPtrType getNormalFlowFunction(n_t Curr,
                                                          n_t Succ) override {
    return getNormalFlowFunctionImpl(Curr, Succ);
  }

  [[nodiscard]] FlowFunctionPtrType
  getCallFlowFunction(n_t CallInst, f_t CalleeFun) override {
    return getCallFlowFunctionImpl(CallInst, CalleeFun);
  }

  [[nodiscard]] FlowFunctionPtrType getRetFlowFunction(n_t CallSite,
                                                       f_t CalleeFun,
                                                       n_t ExitInst,
                                                       n_t RetSite) override {
    return getRetFlowFunctionImpl(CallSite, CalleeFun, ExitInst, RetSite);
  }

  [[nodiscard]] FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           llvm::ArrayRef<f_t> Callees) override {
    return getCallToRetFlowFunctionImpl(CallSite, RetSite, Callees);
  }
};

class DefaultNoAliasIFDSProblem
    : public IFDSTabulationProblem<LLVMIFDSAnalysisDomainDefault>,
      protected detail::IDENoAliasDefaultFlowFunctionsImpl {
public:
  using IFDSTabulationProblem::IFDSTabulationProblem;

  using typename IFDSTabulationProblem::d_t;
  using typename IFDSTabulationProblem::f_t;
  using typename IFDSTabulationProblem::FlowFunctionPtrType;
  using typename IFDSTabulationProblem::l_t;
  using typename IFDSTabulationProblem::n_t;

  [[nodiscard]] FlowFunctionPtrType getNormalFlowFunction(n_t Curr,
                                                          n_t Succ) override {
    return getNormalFlowFunctionImpl(Curr, Succ);
  }

  [[nodiscard]] FlowFunctionPtrType
  getCallFlowFunction(n_t CallInst, f_t CalleeFun) override {
    return getCallFlowFunctionImpl(CallInst, CalleeFun);
  }

  [[nodiscard]] FlowFunctionPtrType getRetFlowFunction(n_t CallSite,
                                                       f_t CalleeFun,
                                                       n_t ExitInst,
                                                       n_t RetSite) override {
    return getRetFlowFunctionImpl(CallSite, CalleeFun, ExitInst, RetSite);
  }

  [[nodiscard]] FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           llvm::ArrayRef<f_t> Callees) override {
    return getCallToRetFlowFunctionImpl(CallSite, RetSite, Callees);
  }
};

} // namespace psr

#endif
