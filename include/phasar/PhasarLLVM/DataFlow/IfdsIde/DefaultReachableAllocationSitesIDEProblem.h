#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_DEFAULTREACHABLEALLOCATIONSITESIDEPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_DEFAULTREACHABLEALLOCATIONSITESIDEPROBLEM_H

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/DefaultNoAliasIDEProblem.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"

// Forward declaration of types for which we only use its pointer or ref type
namespace llvm {
class Value;
class Instruction;
class Function;
} // namespace llvm

namespace psr {

namespace detail {
class IDEReachableAllocationSitesDefaultFlowFunctionsImpl
    : private IDENoAliasDefaultFlowFunctionsImpl {
public:
  using typename IDENoAliasDefaultFlowFunctionsImpl::d_t;
  using typename IDENoAliasDefaultFlowFunctionsImpl::f_t;
  using typename IDENoAliasDefaultFlowFunctionsImpl::FlowFunctionPtrType;
  using typename IDENoAliasDefaultFlowFunctionsImpl::FlowFunctionType;
  using typename IDENoAliasDefaultFlowFunctionsImpl::n_t;

  using IDENoAliasDefaultFlowFunctionsImpl::isFunctionModeled;

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
class DefaultReachableAllocationSitesIDEProblem
    : public IDETabulationProblem<AnalysisDomainTy>,
      protected detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl {
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

  using container_type = typename FlowFunctionType::container_type;

  explicit DefaultReachableAllocationSitesIDEProblem(
      const ProjectIRDBBase<db_t> *IRDB, std::vector<std::string> EntryPoints,
      std::optional<d_t>
          ZeroValue) noexcept(std::is_nothrow_move_constructible_v<d_t>)
      : IDETabulationProblem<AnalysisDomainTy>(IRDB, std::move(EntryPoints),
                                               std::move(ZeroValue)),
        detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl() {}

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

class DefaultReachableAllocationSitesIFDSProblem
    : public IFDSTabulationProblem<LLVMIFDSAnalysisDomainDefault>,
      protected detail::IDEReachableAllocationSitesDefaultFlowFunctionsImpl {
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
