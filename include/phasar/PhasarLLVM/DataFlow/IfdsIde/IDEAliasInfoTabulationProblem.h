#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IDEALIASINFOTABULATIONPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IDEALIASINFOTABULATIONPROBLEM_H

#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"
#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IDEExtendedTaintAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

#include <cassert>

// Forward declaration of types for which we only use its pointer or ref type
namespace llvm {
class Instruction;
class Function;
class StructType;
class Value;
class CallBase;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;
class LLVMTypeHierarchy;

template <typename AnalysisDomainTy,
          typename Container = std::set<typename AnalysisDomainTy::d_t>>
class IDEAliasInfoTabulationProblem
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
  using SourceConfigTy = llvm::SmallPtrSet<const llvm::Value *, 4>;

  using ConfigurationTy = HasNoConfigurationType;

  using FlowFunctionType = FlowFunction<d_t, Container>;
  using FlowFunctionPtrType = typename FlowFunctionType::FlowFunctionPtrType;

  using container_type = typename FlowFunctionType::container_type;

  explicit IDEAliasInfoTabulationProblem(
      const ProjectIRDBBase<db_t> *IRDB, LLVMAliasSet *PT,
      std::vector<std::string> EntryPoints,
      std::optional<d_t>
          ZeroValue) noexcept(std::is_nothrow_move_constructible_v<d_t>)
      : IDETabulationProblem<AnalysisDomainTy, Container>(
            IRDB, std::move(EntryPoints), std::move(ZeroValue)),
        PT(PT) {
    assert(IRDB != nullptr);
  }

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t /*Succ*/) override {
    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
      return this->generateFlowIf(Load, [Load](d_t Source) {
        return Source == Load->getPointerOperand();
      });
    }
    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
      container_type Gen;

      if (Store->getPointerOperand()) {
        assert(PT);
        auto AliasSet = PT.getAliasSet(Store->getPointerOperand(), Store);
        Gen.insert(AliasSet->begin(), AliasSet->end());
        Gen.insert(Store->getValueOperand());

        return this->lambdaFlow(
            [Store, Gen{std::move(Gen)}](d_t Source) -> container_type {
              if (Store->getPointerOperand() == Source) {
                return {};
              }
              if (Store->getValueOperand() == Source) {
                return Gen;
              }

              return {Source};
            });
      }
    }
    if (const auto *UnaryOp = llvm::dyn_cast<llvm::UnaryOperator>(Curr)) {
      return this->generateFlow(UnaryOp, UnaryOp->getOperand(0));
    }
    if (const auto *BinaryOp = llvm::dyn_cast<llvm::BinaryOperator>(Curr)) {
      return this->generateFlowIf(BinaryOp, [BinaryOp](d_t Source) {
        return Source == BinaryOp->getOperand(0) ||
               Source == BinaryOp->getOperand(1);
      });
    }
    if (const auto *GetElementPtr =
            llvm::dyn_cast<llvm::GetElementPtrInst>(Curr)) {
      return this->generateFlow(GetElementPtr,
                                GetElementPtr->getPointerOperand());
    }

    return this->killAllFlows();
  }

  FlowFunctionPtrType getCallFlowFunction(n_t CallInst,
                                          f_t CalleeFun) override {
    if (const auto *CallSite =
            llvm::dyn_cast_or_null<llvm::CallBase>(CallInst)) {
      return mapFactsToCallee(CallSite, CalleeFun,
                              [](d_t Actual, d_t Source) {
                                return Actual == Source &&
                                       Actual->getType()->isPointerTy();
                              },
                              {});
    }

    // TODO: ask Fabian if this is correct. {} caused a seq fault
    return this->killAllFlows();
  }

  static bool canSkipAtContext(const llvm::Value *Val,
                               const llvm::Instruction *Context) noexcept {
    if (const auto *Inst = llvm::dyn_cast<llvm::Instruction>(Val)) {
      /// Mapping instructions between functions is done via the call-FF and
      /// ret-FF
      if (Inst->getFunction() != Context->getFunction()) {
        return true;
      }
      if (Inst->getParent() == Context->getParent() &&
          Context->comesBefore(Inst)) {
        // We will see that inst later
        return true;
      }
      return false;
    }

    if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(Val)) {
      // An argument is only valid in the function it belongs to
      if (Arg->getParent() != Context->getFunction()) {
        return true;
      }
    }
    return false;
  }

  static bool isCompiletimeConstantData(const llvm::Value *Val) noexcept {
    if (const auto *Glob = llvm::dyn_cast<llvm::GlobalVariable>(Val)) {
      // Data cannot flow into the readonly-data section
      return Glob->isConstant();
    }

    return llvm::isa<llvm::Function>(Val) || llvm::isa<llvm::ConstantData>(Val);
  }

  void populateWithMayAliases(container_type &Facts,
                              const llvm::Instruction *Context) const {
    container_type Tmp = Facts;
    for (const auto *Fact : Facts) {
      assert(PT);
      auto Aliases = PT.getAliasSet(Fact);
      for (const auto *Alias : *Aliases) {
        if (canSkipAtContext(Alias, Context)) {
          continue;
        }

        if (isCompiletimeConstantData(Alias)) {
          continue;
        }

        if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Alias)) {
          // Handle at least one level of indirection...
          const auto *PointerOp =
              Load->getPointerOperand()->stripPointerCasts();
          Tmp.insert(PointerOp);
        }

        Tmp.insert(Alias);
      }
    }

    Facts = std::move(Tmp);
  }

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t /*CalleeFun*/,
                                         n_t ExitInst,
                                         n_t /*RetSite*/) override {
    container_type Gen;
    assert(PT);
    auto AliasSet = PT.getAliasSet(CallSite->getOperand(0), CallSite);
    Gen.insert(AliasSet->begin(), AliasSet->end());

    return this->lambdaFlow([CallSite, ExitInst, Gen{std::move(Gen)},
                             this](d_t Source) -> container_type {
      auto PropArg = [](d_t Arg, d_t Source) {
        return Arg == Source && Arg->getType()->isPointerTy();
      };
      // TODO: ask fabian, why a + b and why 2 args
      // auto FactConstructor = [](int a, int b) { return a + b; };
      auto FactConstructor = [](const llvm::Value *TEST) { return TEST; };

      Container Res;

      // TODO: ask fabian why getInt() doesn't work here. Code was copy/pasted
      // from another file, which compiles just fine. Maybe because it was
      // never used/compiled? if (ExitInst->getInt() && ... What about
      // isIntDivRem() ?

      if (ExitInst && LLVMZeroValue::isLLVMZeroValue(Source)) {
        Res.insert(Source);
        return Res;
      }

      if (CallSite->getOperand(0) && llvm::isa<llvm::Constant>(Source)) {
        // Pass global variables as is, if desired
        // Globals could also be actual arguments, then the formal
        // argument needs to be generated below. Need llvm::Constant here
        // to cover also ConstantExpr and ConstantAggregate
        Res.insert(Source);
      }

      const auto *CS = llvm::cast<llvm::CallBase>(CallSite);
      const auto *DestFun = ExitInst->getFunction();
      assert(CS->arg_size() >= DestFun->arg_size());
      assert(CS->arg_size() == DestFun->arg_size() || DestFun->isVarArg());

      llvm::CallBase::const_op_iterator ArgIt = CS->arg_begin();
      llvm::CallBase::const_op_iterator ArgEnd = CS->arg_end();
      llvm::Function::const_arg_iterator ParamIt = DestFun->arg_begin();
      llvm::Function::const_arg_iterator ParamEnd = DestFun->arg_end();

      for (; ParamIt != ParamEnd; ++ParamIt, ++ArgIt) {
        if (ArgIt->get()->getType()->isPointerTy()) {
          if (std::invoke(PropArg, &*ParamIt, Source)) {
            Res.insert(std::invoke(FactConstructor, ArgIt->get()));
          }
        }
      }

      if (ArgIt != ArgEnd) {
        // Over-approximate by trying to add the
        //   alloca [1 x %struct.__va_list_tag], align 16
        // to the results
        // find the allocated %struct.__va_list_tag and generate it

        for (const auto &I : llvm::instructions(DestFun)) {
          if (const auto *Alloc = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
            const auto *AllocTy = Alloc->getAllocatedType();
            if (AllocTy->isArrayTy() && AllocTy->getArrayNumElements() > 0 &&
                AllocTy->getArrayElementType()->isStructTy() &&
                AllocTy->getArrayElementType()->getStructName() ==
                    "struct.__va_list_tag") {
              if (std::invoke(PropArg, Alloc, Source)) {
                std::transform(ArgIt, ArgEnd, std::inserter(Res, Res.end()),
                               FactConstructor);
                break;
              }
            }
          }
        }
      }

      if (const auto *RetInst =
              llvm::dyn_cast<llvm::ReturnInst>(ExitInst->getOperand(0));
          RetInst && RetInst->getReturnValue()) {
        if (std::invoke(
                [](d_t RetVal, d_t Source) {
                  return RetVal == Source ||
                         (LLVMZeroValue::isLLVMZeroValue(Source) &&
                          llvm::isa<llvm::ConstantInt>(RetVal));
                },
                RetInst->getReturnValue(), Source)) {
          Res.insert(std::invoke(FactConstructor, CS));
        }
      }

      std::invoke(
          [CallSite, this](container_type &Res) {
            // Correctly handling return-POIs
            populateWithMayAliases(Res, CallSite);
          },
          Res);

      return Res;
    }

    );
  }

  FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t /*RetSite*/,
                           llvm::ArrayRef<f_t> Callees) override {
    // TODO: alle pointer killen und alle globals
    // Bei declaration only function können wir nicht davon ausgehen, dass der
    // pointer gekillt wird außer bei Funktionen die der analyse bekannt sind.
    //

    // TODO: fix code below
    // If any callee is a declaration, return identity
    if (llvm::any_of(Callees, [](const llvm::Function *Callee) {
          return Callee->isDeclaration();
        })) {
      return this->identityFlow();
    }

    // TODO: fix code below
    if (const auto *CS = llvm::dyn_cast<llvm::CallBase>(CallSite)) {
      return mapFactsAlongsideCallSite(
          CS, [](d_t Arg) { return !Arg->getType()->isPointerTy(); }, false);
    }

    // TODO: ask Fabian if this is correct. {} caused a seq fault
    return this->identityFlow();
  }

private:
  // TODO: Ask Fabian if code below is good, or if commented out code is
  // correct. If LLVMAliasInfoRef, how to initialize this correctly?
  // LLVMAliasInfo PT;
  // LLVMAliasInfoRef PT;
  LLVMAliasInfoRef PT;
};

} // namespace psr

#endif
