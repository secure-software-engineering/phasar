#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IDENOALIASINFOTABULATIONPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IDENOALIASINFOTABULATIONPROBLEM_H

#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/IFDSTabulationProblem.h"
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
  using ProblemAnalysisDomain = AnalysisDomainTy;
  using d_t = typename AnalysisDomainTy::d_t;
  using n_t = typename AnalysisDomainTy::n_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;
  using l_t = typename AnalysisDomainTy::l_t;
  using i_t = typename AnalysisDomainTy::i_t;
  using db_t = typename AnalysisDomainTy::db_t;

  using ConfigurationTy = HasNoConfigurationType;

  using FlowFunctionType = FlowFunction<d_t>;
  using FlowFunctionPtrType = typename FlowFunctionType::FlowFunctionPtrType;

  using IDETabulationProblem<AnalysisDomainTy>::IDETabulationProblem;

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
