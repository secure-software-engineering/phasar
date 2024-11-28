#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IDENOALIASINFOTABULATIONPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IDENOALIASINFOTABULATIONPROBLEM_H

#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"

#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"

// Forward declaration of types for which we only use its pointer or ref type
namespace llvm {
class Instruction;
class Function;
class StructType;
class Value;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;
class LLVMTypeHierarchy;

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class IDENoAliasInfoTabulationProblem
    : public IDETabulationProblem<AnalysisDomainTy, Container> {
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

  using FlowFunctionType = FlowFunction<d_t, Container>;
  using FlowFunctionPtrType = typename FlowFunctionType::FlowFunctionPtrType;

  IDENoAliasInfoTabulationProblem(
      const ProjectIRDBBase<db_t> *IRDB, std::vector<std::string> EntryPoints,
      std::optional<d_t>
          ZeroValue) noexcept(std::is_nothrow_move_constructible_v<d_t>)
      : IDETabulationProblem<AnalysisDomainTy, Container>(
            IRDB, std::move(EntryPoints), std::move(ZeroValue),
            NullAnalysisPrinter<AnalysisDomainTy>::getInstance()) {
    assert(IRDB != nullptr);
  }

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t /*Succ*/) override {
    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
      return generateFlowIf(Load, [Load](d_t Source) {
        return Source == Load->getPointerOperand();
      });
    }
    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
      return strongUpdateStore(Store);
    }
    if (const auto *UnaryOp = llvm::dyn_cast<llvm::UnaryOperator>(Curr)) {
      return generateFlow(UnaryOp, UnaryOp->getOperand(0));
    }
    if (const auto *BinaryOp = llvm::dyn_cast<llvm::BinaryOperator>(Curr)) {
      return generateFlowIf(BinaryOp, [BinaryOp](d_t Source) {
        return Source == BinaryOp->getLeftOp() ||
               Source == BinaryOp->getRightOp();
      });
    }
    if (const auto *GetElementPtr = llvm::dyn_cast<llvm::GEPOperator>(Curr)) {
      return generateFlow(GetElementPtr, GetElementPtr->getPointerOperand());
    }
  }
  FlowFunctionPtrType getCallFlowFunction(n_t CallInst,
                                          f_t CalleeFun) override {
    return mapFactsToCallee(CallInst, CalleeFun, [](d_t Actual, d_t Source) {
      return Actual == Source && Actual->getType()->isPointerTy();
    });
  }
  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t /*CalleeFun*/,
                                         n_t ExitInst,
                                         n_t /*RetSite*/) override {
    return mapFactsToCaller(llvm::cast<llvm::CallBase>(CallSite), ExitInst,
                            [](d_t Param, d_t Source) {
                              return Param == Source &&
                                     Param->getType()->isPointerTy();
                            });
  }
  FlowFunctionPtrType
  getCallToRetFlowFunction(n_t /*CallSite*/, n_t /*RetSite*/,
                           llvm::ArrayRef<f_t> /*Callees*/) override {
    return IDETabulationProblem<AnalysisDomainTy, Container>::identityFlow();
  }

private:
};

} // namespace psr

#endif
