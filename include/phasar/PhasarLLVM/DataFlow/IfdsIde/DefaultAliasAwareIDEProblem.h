/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel, mxHuber and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IDEALIASINFOTABULATIONPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IDEALIASINFOTABULATIONPROBLEM_H

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultNoAliasIDEProblem.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

#include <cassert>

// Forward declaration of types for which we only use its pointer or ref type
namespace llvm {
class Instruction;
class Function;
class Value;
} // namespace llvm

namespace psr {

namespace detail {
class IDEAliasAwareDefaultFlowFunctionsImpl
    : private IDENoAliasDefaultFlowFunctionsImpl {
public:
  using typename IDENoAliasDefaultFlowFunctionsImpl::d_t;
  using typename IDENoAliasDefaultFlowFunctionsImpl::f_t;
  using typename IDENoAliasDefaultFlowFunctionsImpl::FlowFunctionPtrType;
  using typename IDENoAliasDefaultFlowFunctionsImpl::FlowFunctionType;
  using typename IDENoAliasDefaultFlowFunctionsImpl::n_t;

  using IDENoAliasDefaultFlowFunctionsImpl::isFunctionModeled;

  [[nodiscard]] constexpr LLVMAliasIteratorRef getAliasInfo() const noexcept {
    return AS;
  }

  constexpr IDEAliasAwareDefaultFlowFunctionsImpl(
      LLVMAliasIteratorRef AS) noexcept
      : AS(AS) {}

  [[nodiscard]] FlowFunctionPtrType getNormalFlowFunctionImpl(n_t Curr,
                                                              n_t /*Succ*/);
  [[nodiscard]] FlowFunctionPtrType getRetFlowFunctionImpl(n_t CallSite,
                                                           f_t /*CalleeFun*/,
                                                           n_t ExitInst,
                                                           n_t /*RetSite*/);
  using IDENoAliasDefaultFlowFunctionsImpl::getCallFlowFunctionImpl;
  using IDENoAliasDefaultFlowFunctionsImpl::getCallToRetFlowFunctionImpl;

private:
  LLVMAliasIteratorRef AS;
};
} // namespace detail

template <typename AnalysisDomainTy>
class DefaultAliasAwareIDEProblem
    : public IDETabulationProblem<AnalysisDomainTy>,
      protected detail::IDEAliasAwareDefaultFlowFunctionsImpl {
public:
  using typename IDETabulationProblem<AnalysisDomainTy>::db_t;
  using typename IDETabulationProblem<AnalysisDomainTy>::n_t;
  using typename IDETabulationProblem<AnalysisDomainTy>::f_t;
  using typename IDETabulationProblem<AnalysisDomainTy>::d_t;
  using typename IDETabulationProblem<AnalysisDomainTy>::FlowFunctionPtrType;

  using detail::IDEAliasAwareDefaultFlowFunctionsImpl::getAliasInfo;

  /// Constructs an IDETabulationProblem with the usual arguments + alias
  /// information.
  ///
  /// \note It is useful to use an instance of FilteredAliasSet for the alias
  /// information to lower suprious aliases
  explicit DefaultAliasAwareIDEProblem(
      const ProjectIRDBBase<db_t> *IRDB, LLVMAliasIteratorRef AS,
      std::vector<std::string> EntryPoints,
      std::optional<d_t>
          ZeroValue) noexcept(std::is_nothrow_move_constructible_v<d_t>)
      : IDETabulationProblem<AnalysisDomainTy>(IRDB, std::move(EntryPoints),
                                               ZeroValue),
        detail::IDEAliasAwareDefaultFlowFunctionsImpl(AS) {}

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

class DefaultAliasAwareIFDSProblem
    : public IFDSTabulationProblem<LLVMAnalysisDomainDefault>,
      protected detail::IDEAliasAwareDefaultFlowFunctionsImpl {
public:
  using typename IFDSTabulationProblem::d_t;
  using typename IFDSTabulationProblem::f_t;
  using typename IFDSTabulationProblem::FlowFunctionPtrType;
  using typename IFDSTabulationProblem::n_t;

  /// Constructs an IFDSTabulationProblem with the usual arguments + alias
  /// information.
  ///
  /// \note It is useful to use an instance of FilteredAliasSet for the alias
  /// information to lower suprious aliases
  explicit DefaultAliasAwareIFDSProblem(
      const ProjectIRDBBase<db_t> *IRDB, LLVMAliasIteratorRef AS,
      std::vector<std::string> EntryPoints,
      d_t ZeroValue) noexcept(std::is_nothrow_move_constructible_v<d_t>)
      : IFDSTabulationProblem(IRDB, std::move(EntryPoints), ZeroValue),
        detail::IDEAliasAwareDefaultFlowFunctionsImpl(AS) {}

  using detail::IDEAliasAwareDefaultFlowFunctionsImpl::getAliasInfo;

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
