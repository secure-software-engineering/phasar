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

  [[nodiscard]] constexpr LLVMAliasInfoRef getAliasInfo() const noexcept {
    return AS;
  }

  constexpr IDEAliasAwareDefaultFlowFunctionsImpl(LLVMAliasInfoRef AS) noexcept
      : AS(AS) {
    assert(AS && "You must provide an alias information handle!");
  }

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

private:
  LLVMAliasInfoRef AS;
};
} // namespace detail

template <typename AnalysisDomainTy>
class DefaultAliasAwareIDEProblem
    : public IDETabulationProblem<AnalysisDomainTy>,
      protected detail::IDEAliasAwareDefaultFlowFunctionsImpl {
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

  /// Constructs an IDETabulationProblem with the usual arguments + alias
  /// information.
  ///
  /// \note It is useful to use an instance of FilteredAliasSet for the alias
  /// information to lower suprious aliases
  explicit DefaultAliasAwareIDEProblem(
      const ProjectIRDBBase<db_t> *IRDB, LLVMAliasInfoRef AS,
      std::vector<std::string> EntryPoints,
      std::optional<d_t>
          ZeroValue) noexcept(std::is_nothrow_move_constructible_v<d_t>)
      : IDETabulationProblem<AnalysisDomainTy>(IRDB, std::move(EntryPoints),
                                               std::move(ZeroValue)),
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
  /// Constructs an IFDSTabulationProblem with the usual arguments + alias
  /// information.
  ///
  /// \note It is useful to use an instance of FilteredAliasSet for the alias
  /// information to lower suprious aliases
  explicit DefaultAliasAwareIFDSProblem(
      const ProjectIRDBBase<db_t> *IRDB, LLVMAliasInfoRef AS,
      std::vector<std::string> EntryPoints,
      d_t ZeroValue) noexcept(std::is_nothrow_move_constructible_v<d_t>)
      : IFDSTabulationProblem(IRDB, std::move(EntryPoints), ZeroValue),
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

} // namespace psr

#endif
